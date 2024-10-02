#include "ApplyEffectOnHitReceived.h"

#include "Health.h"
#include "LocatorSystem.h"

#include <core/profiling.h>
#include <core/variant/variant_utility.h>

void ApplyEffectOnHitReceived::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_EffectNode", "effectNode"), &ApplyEffectOnHitReceived::SetEffectNode);
	ClassDB::bind_method(D_METHOD("get_EffectNode"), &ApplyEffectOnHitReceived::GetEffectNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "EffectNode", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_EffectNode", "get_EffectNode");

	ClassDB::bind_method(D_METHOD("set_ApplyChance", "applyChance"), &ApplyEffectOnHitReceived::SetApplyChance);
	ClassDB::bind_method(D_METHOD("get_ApplyChance"), &ApplyEffectOnHitReceived::GetApplyChance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ApplyChance"), "set_ApplyChance", "get_ApplyChance");

	ClassDB::bind_method(D_METHOD("set_ChanceModifier", "chanceModifier"), &ApplyEffectOnHitReceived::SetChanceModifier);
	ClassDB::bind_method(D_METHOD("get_ChanceModifier"), &ApplyEffectOnHitReceived::GetChanceModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ChanceModifier"), "set_ChanceModifier", "get_ChanceModifier");

	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &ApplyEffectOnHitReceived::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &ApplyEffectOnHitReceived::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"), "set_ModifierCategories", "get_ModifierCategories");
	TypedArray<StringName> defaultModifierCategories;
	defaultModifierCategories.append("Physical");
	ADD_PROPERTY_DEFAULT("ModifierCategories", defaultModifierCategories);

	ClassDB::bind_method(D_METHOD("set_CooldownModifier", "cooldownModifier"), &ApplyEffectOnHitReceived::SetCooldownModifier);
	ClassDB::bind_method(D_METHOD("get_CooldownModifier"), &ApplyEffectOnHitReceived::GetCooldownModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "CooldownModifier"), "set_CooldownModifier", "get_CooldownModifier");

	ClassDB::bind_method(D_METHOD("set_Cooldown", "cooldown"), &ApplyEffectOnHitReceived::SetCooldown);
	ClassDB::bind_method(D_METHOD("get_Cooldown"), &ApplyEffectOnHitReceived::GetCooldown);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Cooldown"), "set_Cooldown", "get_Cooldown");

	ClassDB::bind_method(D_METHOD("set_RangeModifier", "rangeModifier"), &ApplyEffectOnHitReceived::SetRangeModifier);
	ClassDB::bind_method(D_METHOD("get_RangeModifier"), &ApplyEffectOnHitReceived::GetRangeModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "RangeModifier"), "set_RangeModifier", "get_RangeModifier");

	ClassDB::bind_method(D_METHOD("set_ApplyInRadius", "applyInRadius"), &ApplyEffectOnHitReceived::SetApplyInRadius);
	ClassDB::bind_method(D_METHOD("get_ApplyInRadius"), &ApplyEffectOnHitReceived::GetApplyInRadius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ApplyInRadius"), "set_ApplyInRadius", "get_ApplyInRadius");

	ClassDB::bind_method(D_METHOD("set_RadiusLocatorPool", "radiusLocatorPool"), &ApplyEffectOnHitReceived::SetRadiusLocatorPool);
	ClassDB::bind_method(D_METHOD("get_RadiusLocatorPool"), &ApplyEffectOnHitReceived::GetRadiusLocatorPool);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "RadiusLocatorPool"), "set_RadiusLocatorPool", "get_RadiusLocatorPool");
	ADD_PROPERTY_DEFAULT("RadiusLocatorPool", "Enemies");

	ClassDB::bind_method(D_METHOD("set_RemoveGameObjectAfterNumApplies", "removeGameObjectAfterNumApplies"), &ApplyEffectOnHitReceived::SetRemoveGameObjectAfterNumApplies);
	ClassDB::bind_method(D_METHOD("get_RemoveGameObjectAfterNumApplies"), &ApplyEffectOnHitReceived::GetRemoveGameObjectAfterNumApplies);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "RemoveGameObjectAfterNumApplies"), "set_RemoveGameObjectAfterNumApplies", "get_RemoveGameObjectAfterNumApplies");
	
	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &ApplyEffectOnHitReceived::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &ApplyEffectOnHitReceived::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats"), "set_StatsDisplayType", "get_display_stats_type");

	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &ApplyEffectOnHitReceived::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &ApplyEffectOnHitReceived::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");

	ClassDB::bind_method(D_METHOD("set_DisplayStats", "displayStats"), &ApplyEffectOnHitReceived::SetDisplayStats);
	ClassDB::bind_method(D_METHOD("get_DisplayStats"), &ApplyEffectOnHitReceived::GetDisplayStats);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "DisplayStats"), "set_DisplayStats", "get_DisplayStats");

	ClassDB::bind_method(D_METHOD("get_display_stats"), &ApplyEffectOnHitReceived::get_display_stats);
	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &ApplyEffectOnHitReceived::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &ApplyEffectOnHitReceived::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("onHitTake", "isInvincible", "wasBlocked", "byNode"), &ApplyEffectOnHitReceived::onHitTake);

	ADD_SIGNAL(MethodInfo("Killed", PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
}

void ApplyEffectOnHitReceived::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_enter_tree();
		break;
	}
}

void ApplyEffectOnHitReceived::_enter_tree() {
	PROFILE_FUNCTION()
	init_gameobject_component();
	if (_gameObject.is_valid()) {
		_gameObject->connectToSignal("OnHitTaken", callable_mp(this, &ApplyEffectOnHitReceived::onHitTake));
		initialize_modifiers(this);
		_nextAllowedTime = (float)GameObject::World()->get("current_world_time") + get_cooldown();
	}
}

float ApplyEffectOnHitReceived::get_cooldown() const {
	if(_modifiedCooldown.is_valid()) {
		return 1.0f / _modifiedCooldown->Value();
	}
	return Cooldown;
}

float ApplyEffectOnHitReceived::get_range() const {
	if(_modifiedAreaRadius.is_valid()) {
		return _modifiedAreaRadius->Value();
	}
	return ApplyInRadius;
}

float ApplyEffectOnHitReceived::get_effect_chance() const {
	if(_modifiedChance.is_valid()) {
		return _modifiedChance->Value();
	}
	return ApplyChance;
}

TypedArray<StatisticsValueData> ApplyEffectOnHitReceived::get_display_stats() {
	TypedArray<StatisticsValueData> allStats;
	if(!DisplayStats)
		return allStats;

	allStats.append(
		StatisticsValueData::createNew(
		vformat(tr("%s Chance per Trigger"), GameObject::Global()->call("translateModifierID", ChanceModifier)),
		StatsCategory, ChanceModifier, ApplyChance, get_effect_chance(), ModifierCategories,
		StatisticsValueData::StatisticsFormatTypes::Percentage, StatisticsValueData::StatisticsSpecialInfoTypes::None));

	allStats.append(
		StatisticsValueData::createNew(
		vformat(tr("Max. Distance %s"), GameObject::Global()->call("translateModifierID", RangeModifier, true)),
		StatsCategory, RangeModifier, ApplyInRadius, get_range(), ModifierCategories,
		StatisticsValueData::StatisticsFormatTypes::Distance, StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(get_cooldown() > 0) {
		allStats.append(
			StatisticsValueData::createNew(
			vformat(tr("Cooldown %s"), GameObject::Global()->call("translateModifierID", CooldownModifier, true)),
			StatsCategory, CooldownModifier, Cooldown, get_cooldown(), ModifierCategories,
			StatisticsValueData::StatisticsFormatTypes::Time, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}

	return allStats;
}

void ApplyEffectOnHitReceived::initialize_modifiers(Node *referenceParent) {
	GameObject* referenceGameObject = nullptr;
	if(referenceParent == this)
		referenceGameObject = _gameObject.get();
	else
		referenceGameObject = Object::cast_to<GameObject>(referenceParent->get("_gameObject"));
	if(!ChanceModifier.is_empty()) {
		_modifiedChance.instantiate();
		if(referenceGameObject != nullptr)
			_modifiedChance->_init(ApplyChance, ChanceModifier, referenceGameObject, "");
	}
	if(!CooldownModifier.is_empty()) {
		_modifiedCooldown.instantiate();
		if(referenceGameObject != nullptr)
			_modifiedCooldown->_init(1.0f / Cooldown, CooldownModifier, referenceGameObject, "");
	}
	if(!RangeModifier.is_empty()) {
		_modifiedAreaRadius.instantiate();
		if(referenceGameObject != nullptr)
			_modifiedAreaRadius->_init(ApplyChance, RangeModifier, referenceGameObject, "");
	}
	applyModifierCategories();
}

void ApplyEffectOnHitReceived::applyModifierCategories() {
	PROFILE_FUNCTION()
	if (_modifiedChance.is_valid())
		_modifiedChance->setModifierCategories(ModifierCategories);
	if (_modifiedCooldown.is_valid())
		_modifiedCooldown->setModifierCategories(ModifierCategories);
	if (_modifiedAreaRadius.is_valid())
		_modifiedAreaRadius->setModifierCategories(ModifierCategories);
}

static LocalVector<GameObject*> tempHits;
void ApplyEffectOnHitReceived::onHitTake(bool isInvincible, bool wasBlocked, Node *byNode) {
	float current_world_time = (float)GameObject::World()->get("current_world_time");
	if(_nextAllowedTime > current_world_time)
		return;
	if(RemoveGameObjectAfterNumApplies > 0 && _numTimesApplied >= RemoveGameObjectAfterNumApplies)
		return;
	if(!_gameObject.is_valid() || isInvincible || _gameObject->is_queued_for_deletion() || byNode == nullptr || byNode->is_queued_for_deletion())
		return;
	_nextAllowedTime = current_world_time + get_cooldown();
	GameObject* mySource = _gameObject->get_rootSourceGameObject();
	if(get_range() <= 0) {
		byNode->call("add_effect", EffectNode, mySource);
		int applyNumTimes = VariantUtilityFunctions::ceili(get_effect_chance() - VariantUtilityFunctions::randf());
		for(int i = 0; i < applyNumTimes; i++) {
			byNode->call("add_effect", EffectNode, mySource);
			_numTimesApplied += 1;
		}
		if(RemoveGameObjectAfterNumApplies > 0 && _numTimesApplied >= RemoveGameObjectAfterNumApplies) {
			emit_signal("Killed", Variant());
			_gameObject->queue_free();
		}
	}
	else {
		tempHits.clear();
		LocatorSystem::FillWithGameObjectsInCircle(RadiusLocatorPool, get_gameobject_worldposition(), get_range(), tempHits);
		for(auto gameObj : tempHits) {
			gameObj->add_effect(EffectNode.ptr(), mySource);
			int applyNumTimes = VariantUtilityFunctions::ceili(get_effect_chance() - VariantUtilityFunctions::randf());
			for(int i = 0; i < applyNumTimes; i++) {
				gameObj->add_effect(EffectNode.ptr(), mySource);
				_numTimesApplied += 1;
			}
			if(RemoveGameObjectAfterNumApplies > 0 && _numTimesApplied >= RemoveGameObjectAfterNumApplies) {
				emit_signal("Killed", Variant());
				_gameObject->queue_free();
				return;
			}
		}
	}
}
