#include "ApplyEffectOnCollision.h"

#include <core/variant/variant_utility.h>
#include <core/profiling.h>

void ApplyEffectOnCollision::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_EffectNode", "effectNode"), &ApplyEffectOnCollision::SetEffectNode);
	ClassDB::bind_method(D_METHOD("get_EffectNode"), &ApplyEffectOnCollision::GetEffectNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "EffectNode", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_EffectNode", "get_EffectNode");

	ClassDB::bind_method(D_METHOD("set_ApplyChance", "applyChance"), &ApplyEffectOnCollision::SetApplyChance);
	ClassDB::bind_method(D_METHOD("get_ApplyChance"), &ApplyEffectOnCollision::GetApplyChance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ApplyChance"), "set_ApplyChance", "get_ApplyChance");

	ClassDB::bind_method(D_METHOD("set_ChanceModifier", "chanceModifier"), &ApplyEffectOnCollision::SetChanceModifier);
	ClassDB::bind_method(D_METHOD("get_ChanceModifier"), &ApplyEffectOnCollision::GetChanceModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ChanceModifier"), "set_ChanceModifier", "get_ChanceModifier");

	ClassDB::bind_method(D_METHOD("set_RemoveGameObjectAfterNumApplies", "removeGameObjectAfterNumApplies"), &ApplyEffectOnCollision::SetRemoveGameObjectAfterNumApplies);
	ClassDB::bind_method(D_METHOD("get_RemoveGameObjectAfterNumApplies"), &ApplyEffectOnCollision::GetRemoveGameObjectAfterNumApplies);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "RemoveGameObjectAfterNumApplies"), "set_RemoveGameObjectAfterNumApplies", "get_RemoveGameObjectAfterNumApplies");

	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &ApplyEffectOnCollision::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &ApplyEffectOnCollision::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"), "set_ModifierCategories", "get_ModifierCategories");
	TypedArray<StringName> defaultModifierCategories;
	defaultModifierCategories.append("Physical");
	ADD_PROPERTY_DEFAULT("ModifierCategories", defaultModifierCategories);

	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &ApplyEffectOnCollision::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &ApplyEffectOnCollision::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats"), "set_StatsDisplayType", "get_display_stats_type");

	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &ApplyEffectOnCollision::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &ApplyEffectOnCollision::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");

	ClassDB::bind_method(D_METHOD("set_EffectName", "effectName"), &ApplyEffectOnCollision::SetEffectName);
	ClassDB::bind_method(D_METHOD("get_EffectName"), &ApplyEffectOnCollision::GetEffectName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "EffectName"), "set_EffectName", "get_EffectName");

	ClassDB::bind_method(D_METHOD("get_display_stats"), &ApplyEffectOnCollision::get_display_stats);
	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &ApplyEffectOnCollision::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &ApplyEffectOnCollision::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("collisionWithNode", "node"), &ApplyEffectOnCollision::collisionWithNode);

	ADD_SIGNAL(MethodInfo("Killed", PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
}

void ApplyEffectOnCollision::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_enter_tree();
		break;
	}
}

void ApplyEffectOnCollision::_enter_tree() {
	PROFILE_FUNCTION()
	init_gameobject_component();
	if (_gameObject.is_valid()) {
		_gameObject->connectToSignal("CollisionStarted", callable_mp(this, &ApplyEffectOnCollision::collisionWithNode));
		initialize_modifiers(this);
	}
}

float ApplyEffectOnCollision::get_total_on_hit_chance() const {
	float chance = 0.0f;
	if(_modifiedOnHitChance.is_valid())
		chance += _modifiedOnHitChance->Value();
	if(_modifiedChance.is_valid())
		chance *= _modifiedChance->Value();
	return chance;
}

TypedArray<StatisticsValueData> ApplyEffectOnCollision::get_display_stats() {
	TypedArray<StatisticsValueData> collectedStats;
	String statName = vformat(tr("%s Chance on Hit"), tr(EffectName));
	Array modNames;
	modNames.append("OnHitChance");
	if(!ChanceModifier.is_empty())
		modNames.append(ChanceModifier);
	statName = tr(statName.strip_edges());
		collectedStats.append(
			StatisticsValueData::createNew(
				statName, StatsCategory, modNames,
				ApplyChance, get_total_on_hit_chance(), ModifierCategories,
				StatisticsValueData::StatisticsFormatTypes::Percentage, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	return collectedStats;
}

void ApplyEffectOnCollision::initialize_modifiers(Node *referenceParent) {
	_modifiedOnHitChance = referenceParent->call("createModifiedFloatValue", ApplyChance, "OnHitChance");
	if(!ChanceModifier.is_empty()) {
		_modifiedChance.instantiate();
		_modifiedChance->_initAsMultiplicativeOnly(ChanceModifier, referenceParent->get("_gameObject", nullptr), "");
	}
	applyModifierCategories();
}

void ApplyEffectOnCollision::applyModifierCategories() {
	PROFILE_FUNCTION()
	if(_modifiedChance.is_valid()) {
		_modifiedChance->setModifierCategories(ModifierCategories);
	}
	_modifiedOnHitChance->setModifierCategories(ModifierCategories);
}

void ApplyEffectOnCollision::collisionWithNode(Node *node) {
	if(RemoveGameObjectAfterNumApplies > 0 && _numTimesApplied >= RemoveGameObjectAfterNumApplies)
		return;
	if(!_gameObject.is_valid() || _gameObject->is_queued_for_deletion())
		return;
	PROFILE_FUNCTION()
	float chance = get_total_on_hit_chance();
	if(chance < 1.0f && VariantUtilityFunctions::randf() >= chance)
		return;
	GameObject* mySource = _gameObject->get_rootSourceGameObject();
	node->call("add_effect", EffectNode, mySource);
	_numTimesApplied ++;
	if(RemoveGameObjectAfterNumApplies > 0 && _numTimesApplied >= RemoveGameObjectAfterNumApplies) {
		emit_signal("Killed", Variant());
		if(_gameObject.is_valid() && !_gameObject->is_queued_for_deletion())
			_gameObject->queue_free();
	}
}