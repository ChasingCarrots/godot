#ifndef HOTEXTENSION_GAMEOBJECT_H
#define HOTEXTENSION_GAMEOBJECT_H

#include <scene/2d/node_2d.h>

#include <vector>

#include "Modifier.h"
#include "SafeObjectPointer.h"
#include "ThreadedObjectPool.h"

#include <core/os/time.h>
#include <core/profiling.h>
#include <core/templates/oa_hash_map.h>
#include <scene/main/window.h>

struct SignalCallable {
	StringName SignalName;
	Callable CallableConnection;
};

class GameObject : public Node2D {
    GDCLASS(GameObject, Node2D)

	Variant _spawn_origin;
	Variant _inheritModifierFrom;

	Vector2 _trackedGlobalPosition;

	GameObject * getValidatedInheritModifierFrom() {
		if (_inheritModifierFrom.get_type() != Variant::OBJECT)
			return nullptr;
		Object* gameObjectAsPtr = _inheritModifierFrom.get_validated_object();
		if (gameObjectAsPtr == nullptr)
			return nullptr;
		if (!gameObjectAsPtr->is_class("GameObject"))
			return nullptr;
		return static_cast<GameObject*>(gameObjectAsPtr);
	}
	LocalVector<SignalCallable> _connectedSignals;
	LocalVector<Variant> _sourceTree;
	LocalVector<Modifier*> _modifier;
	OAHashMap<uint32_t, SafeObjectPointer<Node>> _current_effects;

	LocalVector<Node*> _tempNodeArray;
	void populateTempNodesWithAllChildren();

	OAHashMap<StringName, ObjectID> _childNodeWithMethodOrPropertyCache;

	uint32_t calculateHashForModifiedValueCalculation(const StringName& modifierType, const Variant& baseValue, const PackedStringArray& categories) {
		uint32_t calculationHash = modifierType.hash();
		calculationHash *= 16777619;
		calculationHash ^= baseValue.hash();
		for (int i = 0; i < categories.size(); ++i) {
			calculationHash *= 6569;
			calculationHash += categories[i].hash();
		}
		for(const auto mod : _modifier) {
			if(!mod->isRelevant(modifierType, categories))
				continue;
			calculationHash *= 7187;
			calculationHash ^= mod->hash();
		}
		GameObject* inherit_modifier = getValidatedInheritModifierFrom();
		if(inherit_modifier != nullptr) {
			calculationHash *= 	7229;
			calculationHash ^= inherit_modifier->calculateHashForModifiedValueCalculation(modifierType, baseValue, categories);
		}
		return calculationHash;
	}

	static Node* _global;
	static Node* _world;
	static OAHashMap<uint32_t, Ref<ThreadedObjectPool>> EffectObjectPools;
	static OAHashMap<uint32_t, float> CalculatedModifiedFloatValuesCache;
	static OAHashMap<uint32_t, int> CalculatedModifiedIntValuesCache;
	static void InvalidateWorld() {
		_world = nullptr;
		auto poolIter = EffectObjectPools.iter();
		while(poolIter.valid) {
			poolIter.value->ptr()->clear_all_instances();
			poolIter = EffectObjectPools.next_iter(poolIter);
		}
		EffectObjectPools.clear();
		print_line("ModifiedFloatValuesCache: ",CalculatedModifiedFloatValuesCache.get_num_elements(),"  ModifiedIntValuesCache: ",CalculatedModifiedIntValuesCache.get_num_elements() );
		CalculatedModifiedFloatValuesCache.clear();
		CalculatedModifiedIntValuesCache.clear();
	}
protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods();
	void _notification(int p_notification);

public:
	inline static Node* Global() {
		if(_global == nullptr) {
			_global = SceneTree::get_singleton()->get_root()->get_node(NodePath("Global"));
			if(_global != nullptr)
				_global->connect("WorldRemoved", callable_mp_static(&GameObject::InvalidateWorld));
		}
		return _global;
	}
	inline static Node* World() {
		if(_world != nullptr)
			return _world;
		Variant worldVariant = Global()->get("World");
		if (worldVariant.is_null()) {
			print_error("Global.World is null!");
			return nullptr;
		}
		_world = cast_to<Node>(worldVariant);
		return _world;
	}
	static void clearModifiedValueCache() {
		CalculatedModifiedFloatValuesCache.clear();
		CalculatedModifiedIntValuesCache.clear();
	}

    void _connect_child_entered_tree();
    void _exit_tree();
    void _child_entered_tree(Node* childNode);

	static inline GameObject* getGameObjectInParents(Node* node) {
		PROFILE_FUNCTION();
		while(true) {
			if(node == nullptr) return nullptr;
			GameObject* currentNodeGO = dynamic_cast<GameObject*>(node);
			if(currentNodeGO != nullptr)
				return currentNodeGO;
			node = node->get_parent();
		}
	}

    // collective signal system
    void connectToSignal(const StringName &signalName, const Callable &callable);
    void disconnectFromSignal(const StringName& signalName, const Callable &callable);
    bool hasSignal(const StringName& signalName);
    void injectEmitSignal(const StringName& signalName, Array parameters);

    // collective child nodes system
    Node* getChildNodeWithMethod(const StringName& methodName);
    Node* getChildNodeWithSignal(const StringName& signalName);
	Node* getChildNodeWithProperty(const StringName& propertyName);
    void getChildNodesWithMethod(const StringName& methodName, Array fillArray);
	Node* getChildNodeInGroup(const StringName& groupName);

	// modifier system
	void setInheritModifierFrom(GameObject* otherGameObject, bool automaticallyKeepUpdated = false);
	void triggerModifierUpdated(const StringName& modifierType);
	Variant calculateModifiedValue(const StringName& modifierType, Variant baseValue, TypedArray<StringName> categories);
	float getAdditiveModifier(const StringName& modifierType, TypedArray<StringName> categories);
	float getMultiplicativeModifier(const StringName& modifierType, TypedArray<StringName> categories);
	GameObject* getInheritModifierFrom();
	void registerModifier(Modifier* modifier);
	void unregisterModifier(Modifier* modifier);
	Array getModifiers(const StringName& modifierType, TypedArray<StringName> categories);

	// effect system
	Node* add_effect(PackedScene* effectScene, GameObject* externalSource);
	Node* add_effect_from_pool(const Ref<ThreadedObjectPool> &pool, GameObject* externalSource);
	Node* find_effect(const StringName& effectID);

	// sourcetree system
	GameObject* get_rootSourceGameObject();
	void set_sourceGameObject(GameObject* source);

	// spawn origin
	void set_spawn_origin(Node* spawnOrigin);
	Node* get_spawn_origin();

	//ThreadedPool methods
	void managed_by_pool(ThreadedObjectPool* pool);
	void recycle_pooled_object();

	bool vector_approx_equal(const Vector2 &vecA, const Vector2 &vecB) const;

	//tracked global position
	void set_tracked_global_position(const Vector2 &tracked_global_position) {
		PROFILE_FUNCTION()
		_trackedGlobalPosition = tracked_global_position;
	}
	Vector2 get_tracked_global_position() const {
		PROFILE_FUNCTION()

#if DEBUG_ENABLED
		check_tracked_pos();
#endif
		return _trackedGlobalPosition;
	}

	bool check_tracked_pos() const;
};


#endif //HOTEXTENSION_GAMEOBJECT_H
