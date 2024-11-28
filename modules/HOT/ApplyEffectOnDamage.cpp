#include "ApplyEffectOnDamage.h"

#include "Health.h"

#include <core/profiling.h>
#include <core/variant/variant_utility.h>

void ApplyEffectOnDamage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_EffectNode", "effectNode"), &ApplyEffectOnDamage::SetEffectNode);
	ClassDB::bind_method(D_METHOD("get_EffectNode"), &ApplyEffectOnDamage::GetEffectNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "EffectNode", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_EffectNode", "get_EffectNode");

	ClassDB::bind_method(D_METHOD("set_ApplyChance", "applyChance"), &ApplyEffectOnDamage::SetApplyChance);
	ClassDB::bind_method(D_METHOD("get_ApplyChance"), &ApplyEffectOnDamage::GetApplyChance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ApplyChance"), "set_ApplyChance", "get_ApplyChance");

	ClassDB::bind_method(D_METHOD("set_ChanceModifier", "chanceModifier"), &ApplyEffectOnDamage::SetChanceModifier);
	ClassDB::bind_method(D_METHOD("get_ChanceModifier"), &ApplyEffectOnDamage::GetChanceModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ChanceModifier"), "set_ChanceModifier", "get_ChanceModifier");

	ClassDB::bind_method(D_METHOD("set_MultiplyChanceEveryTimeChecked", "multiplyChanceEveryTimeChecked"), &ApplyEffectOnDamage::SetMultiplyChanceEveryTimeChecked);
	ClassDB::bind_method(D_METHOD("get_MultiplyChanceEveryTimeChecked"), &ApplyEffectOnDamage::GetMultiplyChanceEveryTimeChecked);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MultiplyChanceEveryTimeChecked"), "set_MultiplyChanceEveryTimeChecked", "get_MultiplyChanceEveryTimeChecked");

	ClassDB::bind_method(D_METHOD("set_ApplyOnlyOnCritical", "applyOnlyOnCritical"), &ApplyEffectOnDamage::SetApplyOnlyOnCritical);
	ClassDB::bind_method(D_METHOD("get_ApplyOnlyOnCritical"), &ApplyEffectOnDamage::GetApplyOnlyOnCritical);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ApplyOnlyOnCritical"), "set_ApplyOnlyOnCritical", "get_ApplyOnlyOnCritical");

	ClassDB::bind_method(D_METHOD("set_ApplyOnlyWhenActuallyDamaged", "applyOnlyWhenActuallyDamaged"), &ApplyEffectOnDamage::SetApplyOnlyWhenActuallyDamaged);
	ClassDB::bind_method(D_METHOD("get_ApplyOnlyWhenActuallyDamaged"), &ApplyEffectOnDamage::GetApplyOnlyWhenActuallyDamaged);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ApplyOnlyWhenActuallyDamaged"), "set_ApplyOnlyWhenActuallyDamaged", "get_ApplyOnlyWhenActuallyDamaged");

	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &ApplyEffectOnDamage::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &ApplyEffectOnDamage::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"), "set_ModifierCategories", "get_ModifierCategories");
	TypedArray<StringName> defaultModifierCategories;
	defaultModifierCategories.append("Physical");
	ADD_PROPERTY_DEFAULT("ModifierCategories", defaultModifierCategories);

	ClassDB::bind_method(D_METHOD("set_OnlyForDamageCategories", "onlyForDamageCategories"), &ApplyEffectOnDamage::SetOnlyForDamageCategories);
	ClassDB::bind_method(D_METHOD("get_OnlyForDamageCategories"), &ApplyEffectOnDamage::GetOnlyForDamageCategories);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "OnlyForDamageCategories"), "set_OnlyForDamageCategories", "get_OnlyForDamageCategories");

	ClassDB::bind_method(D_METHOD("set_TrackMaxStackQuest", "trackMaxStackQuest"), &ApplyEffectOnDamage::SetTrackMaxStackQuest);
	ClassDB::bind_method(D_METHOD("get_TrackMaxStackQuest"), &ApplyEffectOnDamage::GetTrackMaxStackQuest);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "TrackMaxStackQuest"), "set_TrackMaxStackQuest", "get_TrackMaxStackQuest");

	ClassDB::bind_method(D_METHOD("set_TrackID", "trackID"), &ApplyEffectOnDamage::SetTrackID);
	ClassDB::bind_method(D_METHOD("get_TrackID"), &ApplyEffectOnDamage::GetTrackID);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "TrackID"), "set_TrackID", "get_TrackID");

	ClassDB::bind_method(D_METHOD("set_ChanceForSelfApply", "chanceForSelfApply"), &ApplyEffectOnDamage::SetChanceForSelfApply);
	ClassDB::bind_method(D_METHOD("get_ChanceForSelfApply"), &ApplyEffectOnDamage::GetChanceForSelfApply);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ChanceForSelfApply"), "set_ChanceForSelfApply", "get_ChanceForSelfApply");

	ClassDB::bind_method(D_METHOD("set_SelfApplyNode", "selfApplyNode"), &ApplyEffectOnDamage::SetSelfApplyNode);
	ClassDB::bind_method(D_METHOD("get_SelfApplyNode"), &ApplyEffectOnDamage::GetSelfApplyNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "SelfApplyNode", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_SelfApplyNode", "get_SelfApplyNode");

	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &ApplyEffectOnDamage::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &ApplyEffectOnDamage::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats"), "set_StatsDisplayType", "get_display_stats_type");

	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &ApplyEffectOnDamage::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &ApplyEffectOnDamage::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");

	ClassDB::bind_method(D_METHOD("set_EffectName", "effectName"), &ApplyEffectOnDamage::SetEffectName);
	ClassDB::bind_method(D_METHOD("get_EffectName"), &ApplyEffectOnDamage::GetEffectName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "EffectName"), "set_EffectName", "get_EffectName");

	ClassDB::bind_method(D_METHOD("get_display_stats"), &ApplyEffectOnDamage::get_display_stats);
	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &ApplyEffectOnDamage::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &ApplyEffectOnDamage::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("transformCategories", "onlyWithModifierCategory", "addModifierCategories", "removeModifierCategories", "addDamageCategories", "removeDamageCategories"), &ApplyEffectOnDamage::transformCategories);
	ClassDB::bind_method(D_METHOD("damageWasApplied", "categories", "damageAmount", "applyReturn", "targetNode", "isCritical"), &ApplyEffectOnDamage::damageWasApplied);
	ClassDB::bind_method(D_METHOD("externalDamageWasApplied", "categories", "damageAmount", "applyReturn", "targetNode", "isCritical"), &ApplyEffectOnDamage::externalDamageWasApplied);

	ADD_SIGNAL(MethodInfo("DamageApplied", PropertyInfo(Variant::ARRAY, "damageCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), PropertyInfo(Variant::FLOAT, "damageAmount"), PropertyInfo(Variant::ARRAY, "applyReturn"), PropertyInfo(Variant::OBJECT, "targetNode", PROPERTY_HINT_RESOURCE_TYPE, "GameObject"), PropertyInfo(Variant::BOOL, "critical")));
}

void ApplyEffectOnDamage::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
	}
}

void ApplyEffectOnDamage::_ready() {
	PROFILE_FUNCTION()
	init_gameobject_component();
	if (_gameObject.is_valid()) {
		_gameObject->connectToSignal("DamageApplied", callable_mp(this, &ApplyEffectOnDamage::damageWasApplied));
		Node* effect_instance = EffectNode->instantiate();
		bool valid = false;
		Variant effect_wp_index = effect_instance->get("_weapon_index", &valid);
		if(valid)
			_effect_weapon_index = effect_wp_index;
		_effect_has_was_killed = effect_instance->has_method("was_killed");
		effect_instance->queue_free();
		initialize_modifiers(this);
	}
}

float ApplyEffectOnDamage::get_total_on_hit_chance() const {
	float chance = 0.0f;
	if (_modifiedOnHitChance.is_valid())
		chance += _modifiedOnHitChance->Value();
	if (_modifiedChance.is_valid())
		chance *= _modifiedChance->Value();
	return chance;
}

float ApplyEffectOnDamage::get_total_on_crit_chance() const {
	float chance = 0.0f;
	if (_modifiedOnCritChance.is_valid())
		chance += _modifiedOnCritChance->Value();
	if (_modifiedChance.is_valid())
		chance *= _modifiedChance->Value();
	return chance;
}

TypedArray<StatisticsValueData> ApplyEffectOnDamage::get_display_stats() {
	TypedArray<StatisticsValueData> collectedStats;
	String statName = vformat(tr("%s Chance on Damage"), tr(EffectName));
	String modName = "OnHitChance";
	float statValue = get_total_on_hit_chance();
	if(ApplyOnlyOnCritical) {
		statValue = get_total_on_crit_chance();
		modName = "OnCritChance";
		if(ApplyOnlyWhenActuallyDamaged)
			statName = vformat(tr("%s Chance on Crit Damage"), tr(EffectName));
		else
			statName = vformat(tr("%s Chance on Crit"), tr(EffectName));
	}
	else if(!ApplyOnlyWhenActuallyDamaged) {
		statName = vformat(tr("%s Chance on Hit"), tr(EffectName));
	}

	Array modNames;
	modNames.append(modName);
	if(!ChanceModifier.is_empty())
		modNames.append(ChanceModifier);
	collectedStats.append(
		StatisticsValueData::createNew(
			statName, StatsCategory, modNames,
			ApplyChance, statValue, ModifierCategories,
			StatisticsValueData::StatisticsFormatTypes::Percentage, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	return collectedStats;
}

void ApplyEffectOnDamage::initialize_modifiers(Node *referenceParent) {
	_modifiedOnHitChance = referenceParent->call("createModifiedFloatValue", ApplyChance, "OnHitChance");
	_modifiedOnCritChance = referenceParent->call("createModifiedFloatValue", ApplyChance, "OnCritChance");
	if(!ChanceModifier.is_empty()) {
		_modifiedChance.instantiate();
		_modifiedChance->_initAsMultiplicativeOnly(ChanceModifier, referenceParent->get("_gameObject", nullptr), "");
	}
	applyModifierCategories();
}

void ApplyEffectOnDamage::transformCategories(const String &onlyWithModifierCategory, const TypedArray<StringName> &addModifierCategories, const TypedArray<StringName> &removeModifierCategories, const TypedArray<StringName> &addDamageCategories, const TypedArray<StringName> &removeDamageCategories) {
	if(!ModifierCategories.has(onlyWithModifierCategory))
		return;
	for(const auto& addMod : addModifierCategories) {
		if(!ModifierCategories.has(addMod))
			ModifierCategories.append(addMod);
	}
	for(const auto &remMod : removeModifierCategories) {
		ModifierCategories.erase(remMod);
	}
	applyModifierCategories();
}

void ApplyEffectOnDamage::applyModifierCategories() {
	PROFILE_FUNCTION()
	if(_modifiedChance.is_valid())
		_modifiedChance->setModifierCategories(ModifierCategories);
	if(_modifiedOnHitChance.is_valid())
		_modifiedOnHitChance->setModifierCategories(ModifierCategories);
	if(_modifiedOnCritChance.is_valid())
		_modifiedOnCritChance->setModifierCategories(ModifierCategories);
}

void ApplyEffectOnDamage::damageWasApplied(const TypedArray<String> &categories, float damageAmount, Array applyReturn, GameObject *targetNode, bool isCritical) {
	if(!OnlyForDamageCategories.is_empty()) {
		bool atLeastOneCategoryMatches = false;
		for(const auto& damageCat : categories) {
			if(OnlyForDamageCategories.has(damageCat)) {
				atLeastOneCategoryMatches = true;
				break;
			}
		}
		if(!atLeastOneCategoryMatches)
			return;
	}
	if(ApplyOnlyOnCritical && !isCritical)
		return;
	if(ApplyOnlyWhenActuallyDamaged && (int)applyReturn[1] <= 0)
		return;
	if((int)applyReturn[0] == (int)ApplyDamageResult::Killed && !_effect_has_was_killed)
		// the target was killed in the process, no need to apply effects anymore :)
		return;
	PROFILE_FUNCTION()
	float chance = 0.0f;
	if(isCritical)
		chance = get_total_on_crit_chance();
	else
		chance = get_total_on_hit_chance();
	chance *= MultiplyChanceEveryTimeChecked;
	MultiplyChanceEveryTimeChecked *= MultiplyChanceEveryTimeChecked;

	while(chance > 0) {
		// when the probability is over 1, we don't roll the dice!
		if(chance < 1 && VariantUtilityFunctions::randf() > chance)
			return;
		chance -= 1;
		GameObject* mySource = _gameObject->get_rootSourceGameObject();
		Node* effectNode = Object::cast_to<Node>(targetNode->call("add_effect", EffectNode, mySource));
		for(const auto& cat : categories)
			GameObject::Global()->get("QuestPool").call("notify_effect_applied", cat, _effect_weapon_index);

		if(TrackMaxStackQuest && (int)effectNode->get("_num_stacks") >= (int)effectNode->get("MaxStacks"))
			GameObject::Global()->get("QuestPool").call("notify_max_effect_stacks", TrackID, _effect_weapon_index);
		if(effectNode->has_method("damage_was_received"))
			// this can potentionally lead to multiple damage_was_received calls for the same event.
			// the effect node has to handle that itself, when that is a problem...
			effectNode->call("damage_was_received", damageAmount, _gameObject.get(), -1);
		if(VariantUtilityFunctions::randf() < ChanceForSelfApply) {
			if(SelfApplyNode.is_valid())
				_gameObject->add_effect(SelfApplyNode.ptr(), mySource);
			else
				_gameObject->add_effect(EffectNode.ptr(), mySource);
		}
	}
}

void ApplyEffectOnDamage::externalDamageWasApplied(const TypedArray<String> &categories, float damageAmount, Array applyReturn, GameObject *targetNode, bool isCritical) {
	emit_signal("DamageApplied", categories, damageAmount, applyReturn, targetNode, isCritical);
}