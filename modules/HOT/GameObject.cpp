#include "GameObject.h"

#include <core/profiling.h>
#include <scene/main/window.h>
#include "Modifier.h"

#include <algorithm>

Node* GameObject::_global = nullptr;
Node* GameObject::_world = nullptr;
OAHashMap<uint32_t, Ref<ThreadedObjectPool>> GameObject::EffectObjectPools;

void GameObject::_bind_methods() {
	ClassDB::bind_method(D_METHOD("connectToSignal", "signalName", "callable"), &GameObject::connectToSignal);
	ClassDB::bind_method(D_METHOD("disconnectFromSignal", "signalName", "callable"), &GameObject::disconnectFromSignal);
	ClassDB::bind_method(D_METHOD("hasSignal", "signalName"), &GameObject::hasSignal);
	ClassDB::bind_method(D_METHOD("injectEmitSignal", "signalName", "parameters"), &GameObject::injectEmitSignal);

	ClassDB::bind_method(D_METHOD("getChildNodeWithMethod", "method"), &GameObject::getChildNodeWithMethod);
	ClassDB::bind_method(D_METHOD("getChildNodeWithSignal", "signalName"), &GameObject::getChildNodeWithSignal);
	ClassDB::bind_method(D_METHOD("getChildNodeWithProperty", "propertyName"), &GameObject::getChildNodeWithProperty);
	ClassDB::bind_method(D_METHOD("getChildNodesWithMethod", "method", "fillArray"), &GameObject::getChildNodesWithMethod);
	ClassDB::bind_method(D_METHOD("getChildNodeInGroup", "groupName"), &GameObject::getChildNodeInGroup);

	ClassDB::bind_method(D_METHOD("setInheritModifierFrom", "parentGameObject", "automaticallyKeepUpdated"), &GameObject::setInheritModifierFrom, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("getInheritModifierFrom"), &GameObject::getInheritModifierFrom);
	ClassDB::bind_method(D_METHOD("triggerModifierUpdated", "modifierType"), &GameObject::triggerModifierUpdated);
	ClassDB::bind_method(D_METHOD("calculateModifiedValue", "modifierType", "baseValue", "categories"), &GameObject::calculateModifiedValue);
	ClassDB::bind_method(D_METHOD("getModifiers", "modifierType", "categories"), &GameObject::getModifiers);

	ClassDB::bind_method(D_METHOD("add_effect", "effectScene", "externalSource"), &GameObject::add_effect);
	ClassDB::bind_method(D_METHOD("add_effect_from_pool", "objectPool", "externalSource"), &GameObject::add_effect_from_pool);
	ClassDB::bind_method(D_METHOD("find_effect", "effectID"), &GameObject::find_effect);

	ClassDB::bind_method(D_METHOD("get_rootSourceGameObject"), &GameObject::get_rootSourceGameObject);
	ClassDB::bind_method(D_METHOD("set_sourceGameObject", "source"), &GameObject::set_sourceGameObject);

	ClassDB::bind_method(D_METHOD("set_spawn_origin", "spawn_origin_node"), &GameObject::set_spawn_origin);
	ClassDB::bind_method(D_METHOD("get_spawn_origin"), &GameObject::get_spawn_origin);

	ClassDB::bind_method(D_METHOD("_connect_child_entered_tree"), &GameObject::_connect_child_entered_tree);

	ADD_SIGNAL(MethodInfo("ModifierUpdated", PropertyInfo(Variant::STRING_NAME, "type")));
	ADD_SIGNAL(MethodInfo("Removed", PropertyInfo(Variant::OBJECT, "type", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
}

void GameObject::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_ENTER_TREE:
			// needs to be called deferred, so that the actual "child_entered_tree" doesn't
			// directly get called for all the existing children!
			// (especially problematic when removing and readding a gameobject)
			call_deferred("_connect_child_entered_tree");
			break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
			break;
	}
}

void GameObject::_connect_child_entered_tree() {
	if (is_inside_tree() && !Node::is_connected("child_entered_tree", callable_mp(this, &GameObject::_child_entered_tree)))
		connect("child_entered_tree", callable_mp(this, &GameObject::_child_entered_tree));
}

void GameObject::_exit_tree() {
	if (Node::is_connected("child_entered_tree", callable_mp(this, &GameObject::_child_entered_tree)))
		disconnect("child_entered_tree", callable_mp(this, &GameObject::_child_entered_tree));
	emit_signal("Removed", this);
}

void GameObject::_child_entered_tree(Node* childNode) {
	PROFILE_FUNCTION()
	// we check all the signal connections, so that we can connect signals
	// even when the child that has the signal was added after the one that
	// wants to connect to it!
	for(auto connectedSignal : _connectedSignals) {
		if(!connectedSignal.CallableConnection.is_valid())
			continue;
		if(childNode->has_signal(connectedSignal.SignalName)) {
			// don't connect the node to itself
			if(connectedSignal.CallableConnection.get_object() == childNode)
				continue;
			childNode->connect(connectedSignal.SignalName, connectedSignal.CallableConnection);
		}
	}
}

void fillArrayWithChildrenOfNode(Node* node, LocalVector<Node*>& array) {
	for(int i=0; i < node->get_child_count(); ++i)
		array.push_back(node->get_child(i));
}

void GameObject::populateTempNodesWithAllChildren() {
	_tempNodeArray.clear();
	fillArrayWithChildrenOfNode(this, _tempNodeArray);
	uint32_t currentIndex = 0;
	while(currentIndex < _tempNodeArray.size()) {
		if(_tempNodeArray[currentIndex]->is_queued_for_deletion())
		{
			_tempNodeArray.remove_at_unordered(currentIndex);
			continue;
		}
		fillArrayWithChildrenOfNode(_tempNodeArray[currentIndex], _tempNodeArray);
		currentIndex += 1;
	}
}

void GameObject::connectToSignal(String signalName, Callable callable) {
	PROFILE_FUNCTION()
	_connectedSignals.push_back({
			signalName,
			callable
	});
	Object* callableObject = callable.get_object();
	populateTempNodesWithAllChildren();
	for(auto child : _tempNodeArray) {
		// don't connect the node to itself
		if(child != callableObject && child->has_signal(signalName))
			child->connect(signalName, callable);
	}
}

void GameObject::disconnectFromSignal(String signalName, Callable callable) {
	PROFILE_FUNCTION()
	int connectedSignalsIndex = 0;
	auto connectedSignalsIter = _connectedSignals.begin();
	while (connectedSignalsIter != _connectedSignals.end()) {
		if(connectedSignalsIter->SignalName == signalName && connectedSignalsIter->CallableConnection == callable)
			_connectedSignals.remove_at_unordered(connectedSignalsIndex);
		else {
			++connectedSignalsIter;
			++connectedSignalsIndex;
		}
	}
	populateTempNodesWithAllChildren();
	for(auto child : _tempNodeArray) {
		if(child->has_signal(signalName))
			child->disconnect(signalName, callable);
	}
}

bool GameObject::hasSignal(String signalName) {
	PROFILE_FUNCTION()
	populateTempNodesWithAllChildren();
	for(auto child : _tempNodeArray) {
		if(child->has_signal(signalName))
			return true;
	}
	return false;
}

void GameObject::injectEmitSignal(String signalName, Array parameters) {
	PROFILE_FUNCTION()
	for(auto& signalConnection : _connectedSignals) {
		if(signalConnection.SignalName == signalName && signalConnection.CallableConnection.is_valid())
			signalConnection.CallableConnection.callv(parameters);
	}
}

Node* GameObject::getChildNodeWithMethod(const StringName& methodName) {
	PROFILE_FUNCTION()

	ObjectID foundObjectID;
	if (_childNodeWithMethodOrPropertyCache.lookup(methodName, foundObjectID)) {
		auto cachedObject = ObjectDB::get_instance(foundObjectID);
		if (cachedObject != nullptr) {
			auto cachedNode = Object::cast_to<Node>(cachedObject);
			if (cachedNode != nullptr && !cachedNode->is_queued_for_deletion())
				return cachedNode;
		}
		// this cached method's object is not valid anymore...
		_childNodeWithMethodOrPropertyCache.remove(methodName);
	}

	_tempNodeArray.clear();
	fillArrayWithChildrenOfNode(this, _tempNodeArray);
	uint32_t currentIndex = 0;
	while (currentIndex < _tempNodeArray.size()) {
		if (_tempNodeArray[currentIndex]->is_queued_for_deletion()) {
			currentIndex += 1;
			continue;
		}
		if (_tempNodeArray[currentIndex]->has_method(methodName)) {
			_childNodeWithMethodOrPropertyCache.insert(methodName, _tempNodeArray[currentIndex]->get_instance_id());
			return _tempNodeArray[currentIndex];
		}
		fillArrayWithChildrenOfNode(_tempNodeArray[currentIndex], _tempNodeArray);
		currentIndex += 1;
	}
	return nullptr;
}

Node* GameObject::getChildNodeWithSignal(const StringName& signalName) {
	PROFILE_FUNCTION()
	_tempNodeArray.clear();
	fillArrayWithChildrenOfNode(this, _tempNodeArray);
	uint32_t currentIndex = 0;
	while(currentIndex < _tempNodeArray.size()) {
		if(_tempNodeArray[currentIndex]->is_queued_for_deletion())
		{
			currentIndex +=1;
			continue;
		}
		if(_tempNodeArray[currentIndex]->has_signal(signalName))
			return _tempNodeArray[currentIndex];
		fillArrayWithChildrenOfNode(_tempNodeArray[currentIndex], _tempNodeArray);
		currentIndex += 1;
	}
	return nullptr;
}

Node* GameObject::getChildNodeWithProperty(const StringName& propertyName) {
	PROFILE_FUNCTION()
	ObjectID foundObjectID;
	if (_childNodeWithMethodOrPropertyCache.lookup(propertyName, foundObjectID)) {
		auto cachedObject = ObjectDB::get_instance(foundObjectID);
		if (cachedObject != nullptr) {
			auto cachedNode = Object::cast_to<Node>(cachedObject);
			if (cachedNode != nullptr && !cachedNode->is_queued_for_deletion())
				return cachedNode;
		}
		// this cached method's object is not valid anymore...
		_childNodeWithMethodOrPropertyCache.remove(propertyName);
	}

	_tempNodeArray.clear();
	fillArrayWithChildrenOfNode(this, _tempNodeArray);
	uint32_t currentIndex = 0;
	while(currentIndex < _tempNodeArray.size()) {
		if(_tempNodeArray[currentIndex]->is_queued_for_deletion())
		{
			currentIndex += 1;
			continue;
		}
		bool valid = false;
		_tempNodeArray[currentIndex]->get(propertyName, &valid);
		if(valid) {
			_childNodeWithMethodOrPropertyCache.insert(propertyName, _tempNodeArray[currentIndex]->get_instance_id());
			return _tempNodeArray[currentIndex];
		}
		fillArrayWithChildrenOfNode(_tempNodeArray[currentIndex], _tempNodeArray);
		currentIndex += 1;
	}
	return nullptr;
}

void GameObject::getChildNodesWithMethod(const StringName& methodName, Array fillArray) {
	PROFILE_FUNCTION()
	populateTempNodesWithAllChildren();
	for(auto child : _tempNodeArray) {
		if(child->has_method(methodName))
			fillArray.append(child);
	}
}

Node* GameObject::getChildNodeInGroup(const StringName& groupName) {
	PROFILE_FUNCTION()
	_tempNodeArray.clear();
	fillArrayWithChildrenOfNode(this, _tempNodeArray);
	uint32_t currentIndex = 0;
	while(currentIndex < _tempNodeArray.size()) {
		if(_tempNodeArray[currentIndex]->is_queued_for_deletion())
		{
			currentIndex += 1;
			continue;
		}
		if(_tempNodeArray[currentIndex]->is_in_group(groupName))
			return _tempNodeArray[currentIndex];
		fillArrayWithChildrenOfNode(_tempNodeArray[currentIndex], _tempNodeArray);
		currentIndex += 1;
	}
	return nullptr;
}

void GameObject::setInheritModifierFrom(GameObject *otherGameObject, bool automaticallyKeepUpdated) {
	PROFILE_FUNCTION()
	GameObject* validatedCurrentInheritModifierFrom = getValidatedInheritModifierFrom();
	if(automaticallyKeepUpdated && validatedCurrentInheritModifierFrom != nullptr && !validatedCurrentInheritModifierFrom->is_queued_for_deletion())
		validatedCurrentInheritModifierFrom->disconnect("ModifierUpdated", callable_mp(this, &GameObject::triggerModifierUpdated));

	bool isOtherGameObjectValid = otherGameObject != nullptr && !otherGameObject->is_queued_for_deletion();
	_inheritModifierFrom = otherGameObject;

	if(automaticallyKeepUpdated && isOtherGameObjectValid)
		otherGameObject->connect("ModifierUpdated", callable_mp(this, &GameObject::triggerModifierUpdated));
	triggerModifierUpdated("ALL");
}

GameObject *GameObject::getInheritModifierFrom() {
	return getValidatedInheritModifierFrom();
}

void GameObject::triggerModifierUpdated(String modifierType) {
	PROFILE_FUNCTION();
	emit_signal("ModifierUpdated", modifierType);
}

Variant GameObject::calculateModifiedValue(String modifierType, Variant baseValue, TypedArray<String> categories) {
	PROFILE_FUNCTION();
	int baseValueInt = baseValue;
	float baseValueFloat = baseValue;
	float multiplier = 1;
	PackedStringArray categoriesAsPackedStr;
	categoriesAsPackedStr.resize(categories.size());
	for (int i = 0; i < categories.size(); ++i)
		categoriesAsPackedStr.set(i, categories[i]);

	for(auto modifier : _modifier) {
		if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
			continue ;
		baseValueInt += (int)modifier->getAdditiveModifier();
		baseValueFloat += modifier->getAdditiveModifier();
		multiplier += modifier->getMultiplierModifier();
	}

	GameObject* validatedInheritModifierFrom = getValidatedInheritModifierFrom();
	if(validatedInheritModifierFrom != nullptr && !validatedInheritModifierFrom->is_queued_for_deletion()) {
		for(auto modifier : validatedInheritModifierFrom->_modifier) {
			if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
				continue ;
			baseValueInt += (int)modifier->getAdditiveModifier();
			baseValueFloat += modifier->getAdditiveModifier();
			multiplier += modifier->getMultiplierModifier();
		}
	}

	if(baseValue.get_type() == Variant::FLOAT) {
		if(multiplier < 0) return 0.0f;
		return baseValueFloat * multiplier;
	}
	if(multiplier < 0) return 0;
	return (int)(baseValueInt * multiplier);
}

float GameObject::getAdditiveModifier(String modifierType, TypedArray<String> categories) {
	PROFILE_FUNCTION();
	PackedStringArray categoriesAsPackedStr;
	categoriesAsPackedStr.resize(categories.size());
	for (int i = 0; i < categories.size(); ++i)
		categoriesAsPackedStr.set(i, categories[i]);
	float additiveModifierSum = 0;
	for(auto modifier : _modifier) {
		if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
			continue ;
		additiveModifierSum += modifier->getAdditiveModifier();
	}

	GameObject* validatedInheritModifierFrom = getValidatedInheritModifierFrom();
	if(validatedInheritModifierFrom != nullptr && !validatedInheritModifierFrom->is_queued_for_deletion()) {
		for(auto modifier : validatedInheritModifierFrom->_modifier) {
			if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
				continue ;
			additiveModifierSum += modifier->getAdditiveModifier();
		}
	}

	return additiveModifierSum;
}

float GameObject::getMultiplicativeModifier(String modifierType, TypedArray<String> categories) {
	PROFILE_FUNCTION();
	PackedStringArray categoriesAsPackedStr;
	categoriesAsPackedStr.resize(categories.size());
	for (int i = 0; i < categories.size(); ++i)
		categoriesAsPackedStr.set(i, categories[i]);

	float multiplierSum = 1;

	for(auto modifier : _modifier) {
		if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
			continue ;
		multiplierSum += modifier->getMultiplierModifier();
	}

	GameObject* validatedInheritModifierFrom = getValidatedInheritModifierFrom();
	if(validatedInheritModifierFrom != nullptr && !validatedInheritModifierFrom->is_queued_for_deletion()) {
		for(auto modifier : validatedInheritModifierFrom->_modifier) {
			if(!modifier->isRelevant(modifierType, categoriesAsPackedStr))
				continue ;
			multiplierSum += modifier->getMultiplierModifier();
		}
	}

	return multiplierSum;
}

void GameObject::registerModifier(Modifier *modifier) {
	if(_modifier.find(modifier) != -1)
		return;
	_modifier.push_back(modifier);
}

void GameObject::unregisterModifier(Modifier *modifier) {
	auto modIndex = _modifier.find(modifier);
	if(modIndex == -1)
		return;

	_modifier.remove_at_unordered(modIndex);
	if(!is_queued_for_deletion()) {
		// the modified values have changed after this and we
		// would have to trigger the update via script anyways,
		// so let's just do it here...
		triggerModifierUpdated(modifier->_modifiedType);
	}
}

Array GameObject::getModifiers(String modifierType, TypedArray<String> categories) {
	PROFILE_FUNCTION();
	PackedStringArray categoriesAsPackedStr;
	categoriesAsPackedStr.resize(categories.size());
	for (int i = 0; i < categories.size(); ++i)
		categoriesAsPackedStr.set(i, categories[i]);

	Array returnArray;

	for(auto modifier : _modifier)
		if(modifier->isRelevant(modifierType, categoriesAsPackedStr))
			returnArray.append(modifier);

	return returnArray;
}

Node* GameObject::add_effect(PackedScene *effectScene, GameObject *externalSource) {
	PROFILE_FUNCTION();
	if(effectScene == nullptr)
		return nullptr;
	Ref<ThreadedObjectPool> effect_object_pool;
	if(!EffectObjectPools.lookup(effectScene->get_path().hash(), effect_object_pool)) {
		effect_object_pool.instantiate();
		effect_object_pool->init_with_scene_res(effectScene, 999, ThreadedObjectPool::ReturnNull);
		EffectObjectPools.insert(effectScene->get_path().hash(), effect_object_pool);
	}
	return add_effect_from_pool(effect_object_pool, externalSource);
}

Node *GameObject::add_effect_from_pool(Ref<ThreadedObjectPool> effect_object_pool, GameObject *externalSource) {
	PROFILE_FUNCTION();
	Node* effect_instance = effect_object_pool->get_instance_unthreaded();

	// this can happen in rare cases (max number of instances reached)
	// we return nullptr here and the script will run into problems...
	// but it should almost never happen, because we have a really
	// large maxNumberOfInstances for the effect pools!
	if(effect_instance == nullptr) {
		print_error("Effect pool ran out of instances! (shouldn't really happen, we have a huge buffer of instances!)");
		return nullptr;
	}

	String effectID = effect_instance->call("get_effectID");
	Variant questPool = Global()->get("QuestPool");
	if(questPool.has_method("notify_effect_applied"))
		questPool.call("notify_effect_applied", effectID);

	SafeObjectPointer<Node> already_applied_effect;
	if(_current_effects.lookup(effectID.hash(), already_applied_effect)) {
		if(already_applied_effect.is_valid() &&
			already_applied_effect->get_parent() == this)
		{
			already_applied_effect->call("add_additional_effect", effect_instance);
			effect_object_pool->return_instance(effect_instance);
			return already_applied_effect.get_nocheck();
		}
		// when there is an invalid effect in there at that position,
		// it has to be removed (otherwise .insert() will add another instance!)
		_current_effects.remove(effectID.hash());
	}

	effect_instance->call("set_externalSource", externalSource);
	add_child(effect_instance);
	_current_effects.insert(effectID.hash(), SafeObjectPointer(effect_instance));
	return effect_instance;
}

Node* GameObject::find_effect(String effectID) {
	PROFILE_FUNCTION();

	SafeObjectPointer<Node> already_applied_effect;
	if(_current_effects.lookup(effectID.hash(), already_applied_effect) &&
		already_applied_effect.is_valid() &&
		already_applied_effect->get_parent() == this)
		return already_applied_effect.get_nocheck();

	return nullptr;
}

GameObject* GameObject::get_rootSourceGameObject() {
	PROFILE_FUNCTION()
	if(_sourceTree.is_empty())
		return this;

	auto sourceIter = --_sourceTree.end();
	while(sourceIter != --_sourceTree.begin()) {
		auto sourceObj = sourceIter->get_validated_object();
		if(sourceObj != nullptr) {
			return dynamic_cast<GameObject *>(sourceObj);
		}
		--sourceIter;
	}
	return this;
}

void GameObject::set_sourceGameObject(GameObject *source) {
	_sourceTree.clear();
	if(source == nullptr)
		return;
	_sourceTree.push_back(source);
	for(auto sourceSource : source->_sourceTree)
		_sourceTree.push_back(sourceSource);
}

void GameObject::set_spawn_origin(Node *spawnOrigin) {
	_spawn_origin = spawnOrigin;
}

Node* GameObject::get_spawn_origin() {
	if(_spawn_origin.is_null())
		return nullptr;
	return (Node*)_spawn_origin.get_validated_object();
}

