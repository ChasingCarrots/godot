#include "AreaOfEffect.h"

#include "StaticValueHelper.h"

#include <core/object/script_language.h>
#include <core/variant/variant_utility.h>

LocalVector<AreaOfEffect*> AreaOfEffect::_allAreaOfEffects;

void AreaOfEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_ApplyDamage", "applyDamage"), &AreaOfEffect::SetApplyDamage);
	ClassDB::bind_method(D_METHOD("get_ApplyDamage"), &AreaOfEffect::GetApplyDamage);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "ApplyDamage"), "set_ApplyDamage", "get_ApplyDamage");
	ClassDB::bind_method(D_METHOD("set_RankDamageModifierMethod", "rankDamageModifierMethod"), &AreaOfEffect::SetRankDamageModifierMethod);
	ClassDB::bind_method(D_METHOD("get_RankDamageModifierMethod"), &AreaOfEffect::GetRankDamageModifierMethod);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "RankDamageModifierMethod"), "set_RankDamageModifierMethod", "get_RankDamageModifierMethod");
	ClassDB::bind_method(D_METHOD("set_ApplyNode", "applyNode"), &AreaOfEffect::SetApplyNode);
	ClassDB::bind_method(D_METHOD("get_ApplyNode"), &AreaOfEffect::GetApplyNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ApplyNode", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_ApplyNode", "get_ApplyNode");
	ClassDB::bind_method(D_METHOD("set_TriggerHitOnChildren", "triggerHitOnChildren"), &AreaOfEffect::SetTriggerHitOnChildren);
	ClassDB::bind_method(D_METHOD("get_TriggerHitOnChildren"), &AreaOfEffect::GetTriggerHitOnChildren);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "TriggerHitOnChildren"), "set_TriggerHitOnChildren", "get_TriggerHitOnChildren");
	ADD_PROPERTY_DEFAULT("TriggerHitOnChildren", true);
	ClassDB::bind_method(D_METHOD("set_TriggerHitOnParentGameObject", "triggerHitOnParentGameObject"), &AreaOfEffect::SetTriggerHitOnParentGameObject);
	ClassDB::bind_method(D_METHOD("get_TriggerHitOnParentGameObject"), &AreaOfEffect::GetTriggerHitOnParentGameObject);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "TriggerHitOnParentGameObject"), "set_TriggerHitOnParentGameObject", "get_TriggerHitOnParentGameObject");
	ADD_PROPERTY_DEFAULT("TriggerHitOnParentGameObject", false);
	ClassDB::bind_method(D_METHOD("set_LocatorPoolName", "locatorPoolName"), &AreaOfEffect::SetLocatorPoolName);
	ClassDB::bind_method(D_METHOD("get_LocatorPoolName"), &AreaOfEffect::GetLocatorPoolName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "LocatorPoolName"), "set_LocatorPoolName", "get_LocatorPoolName");
	ADD_PROPERTY_DEFAULT("LocatorPoolName", "Player");
	ClassDB::bind_method(D_METHOD("set_Radius", "radius"), &AreaOfEffect::SetRadius);
	ClassDB::bind_method(D_METHOD("get_Radius"), &AreaOfEffect::GetRadius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Radius"), "set_Radius", "get_Radius");
	ADD_PROPERTY_DEFAULT("Radius", 30);
	ClassDB::bind_method(D_METHOD("set_TriggerEverySeconds", "triggerEverySeconds"), &AreaOfEffect::SetTriggerEverySeconds);
	ClassDB::bind_method(D_METHOD("get_TriggerEverySeconds"), &AreaOfEffect::GetTriggerEverySeconds);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "TriggerEverySeconds"), "set_TriggerEverySeconds", "get_TriggerEverySeconds");
	ClassDB::bind_method(D_METHOD("set_ProbabilityToApply", "probabilityToApply"), &AreaOfEffect::SetProbabilityToApply);
	ClassDB::bind_method(D_METHOD("get_ProbabilityToApply"), &AreaOfEffect::GetProbabilityToApply);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ProbabilityToApply"), "set_ProbabilityToApply", "get_ProbabilityToApply");
	ADD_PROPERTY_DEFAULT("ProbabilityToApply", 1.0f);
	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &AreaOfEffect::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &AreaOfEffect::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_ModifierCategories", "get_ModifierCategories");
	ClassDB::bind_method(D_METHOD("set_DamageCategories", "damageCategories"), &AreaOfEffect::SetDamageCategories);
	ClassDB::bind_method(D_METHOD("get_DamageCategories"), &AreaOfEffect::GetDamageCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "DamageCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_DamageCategories", "get_DamageCategories");
	ClassDB::bind_method(D_METHOD("set_UseModifiedArea", "useModifiedArea"), &AreaOfEffect::SetUseModifiedArea);
	ClassDB::bind_method(D_METHOD("get_UseModifiedArea"), &AreaOfEffect::GetUseModifiedArea);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "UseModifiedArea"), "set_UseModifiedArea", "get_UseModifiedArea");
	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &AreaOfEffect::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &AreaOfEffect::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");
	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &AreaOfEffect::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_StatsDisplayType"), &AreaOfEffect::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats,AbilityStats"), "set_StatsDisplayType", "get_StatsDisplayType");

	ClassDB::bind_method(D_METHOD("set_RemainingTimeToNextTrigger", "remainingTimeToNextTrigger"), &AreaOfEffect::SetRemainingTimeToNextTrigger);
	ClassDB::bind_method(D_METHOD("get_RemainingTimeToNextTrigger"), &AreaOfEffect::GetRemainingTimeToNextTrigger);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_remainingTimeToNextTrigger", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_RemainingTimeToNextTrigger", "get_RemainingTimeToNextTrigger");
	ClassDB::bind_method(D_METHOD("set_IsHarmless", "isHarmless"), &AreaOfEffect::SetIsHarmless);
	ClassDB::bind_method(D_METHOD("get_IsHarmless"), &AreaOfEffect::GetIsHarmless);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_is_harmless", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_IsHarmless", "get_IsHarmless");
	ClassDB::bind_method(D_METHOD("set_ModifiedDamage", "modifiedDamage"), &AreaOfEffect::SetModifiedDamage);
	ClassDB::bind_method(D_METHOD("get_ModifiedDamage"), &AreaOfEffect::GetModifiedDamage);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedDamage", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedDamage", "get_ModifiedDamage");
	ClassDB::bind_method(D_METHOD("set_DamageModifier", "damageModifier"), &AreaOfEffect::SetDamageModifier);
	ClassDB::bind_method(D_METHOD("get_DamageModifier"), &AreaOfEffect::GetDamageModifier);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_damageModifier", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_DamageModifier", "get_DamageModifier");
	ClassDB::bind_method(D_METHOD("set_ModifiedAttackSpeed", "modifiedAttackSpeed"), &AreaOfEffect::SetModifiedAttackSpeed);
	ClassDB::bind_method(D_METHOD("get_ModifiedAttackSpeed"), &AreaOfEffect::GetModifiedAttackSpeed);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedAttackSpeed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedAttackSpeed", "get_ModifiedAttackSpeed");
	ClassDB::bind_method(D_METHOD("set_ModifiedArea", "modifiedArea"), &AreaOfEffect::SetModifiedArea);
	ClassDB::bind_method(D_METHOD("get_ModifiedArea"), &AreaOfEffect::GetModifiedArea);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedArea", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedArea", "get_ModifiedArea");
	ClassDB::bind_method(D_METHOD("set_WeaponIndex", "weaponIndex"), &AreaOfEffect::SetWeaponIndex);
	ClassDB::bind_method(D_METHOD("get_WeaponIndex"), &AreaOfEffect::GetWeaponIndex);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_weapon_index", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_WeaponIndex", "get_WeaponIndex");

	ADD_SIGNAL(MethodInfo("DamageApplied", PropertyInfo(Variant::ARRAY, "damageCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), PropertyInfo(Variant::FLOAT, "damageAmount"), PropertyInfo(Variant::ARRAY, "applyReturn"), PropertyInfo(Variant::OBJECT, "targetNode", PROPERTY_HINT_RESOURCE_TYPE, "GameObject"), PropertyInfo(Variant::BOOL, "critical")));

	ClassDB::bind_static_method("AreaOfEffect", D_METHOD("updateAllAreaOfEffects", "delta"), &AreaOfEffect::UpdateAllAreaOfEffects);

	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &AreaOfEffect::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &AreaOfEffect::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("attackSpeedWasChanged", "oldValue", "newValue"), &AreaOfEffect::attackSpeedWasChanged);
	ClassDB::bind_method(D_METHOD("set_harmless", "harmless"), &AreaOfEffect::set_harmless);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &AreaOfEffect::get_display_stats_type);
	ClassDB::bind_method(D_METHOD("get_display_stats"), &AreaOfEffect::get_display_stats);
}

void AreaOfEffect::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_enter_tree();
		break;
		case NOTIFICATION_EXIT_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_exit_tree();
		break;
	}
}

void AreaOfEffect::_exit_tree() {
	_allAreaOfEffects.erase(this);
}

void AreaOfEffect::_enter_tree() {
	_allAreaOfEffects.push_back(this);
}

void AreaOfEffect::_ready() {
	PROFILE_FUNCTION();
	init_gameobject_component();
	if(!_gameObject.is_valid()) return;
	initialize_modifiers(this);
}

static LocalVector<GameObject*> tempHits;
void AreaOfEffect::UpdateAllAreaOfEffects(float delta) {
	PROFILE_FUNCTION();
	LocatorSystem* locators = nullptr;
	for(auto* area : _allAreaOfEffects) {
		area->_remainingTimeToNextTrigger -= delta;
		if(area->TriggerEverySeconds <= 0 || area->_remainingTimeToNextTrigger < 0) {
			if(area->_is_harmless || area->_modifiedAttackSpeed->Value() == 0)
				continue;
			area->_remainingTimeToNextTrigger += 1.0f / area->_modifiedAttackSpeed->Value();
			Vector2 position = area->_positionProvider->call("get_worldPosition");

			tempHits.clear();
			if(area->UseModifiedArea)
				LocatorSystem::FillWithGameObjectsInCircle(area->LocatorPoolName, position, area->_modifiedArea->Value(), tempHits);
			else
				LocatorSystem::FillWithGameObjectsInCircle(area->LocatorPoolName, position, area->Radius, tempHits);

			GameObject* mySource = area->_gameObject->get_rootSourceGameObject();
			for(auto hitObj : tempHits) {
				if(area->ProbabilityToApply < 1 && VariantUtilityFunctions::randf() > area->ProbabilityToApply)
					continue;
				if(area->_modifiedDamage->Value() > 0) {
					Node* healthComp = hitObj->getChildNodeWithMethod("applyDamage");
					if(healthComp != nullptr) {
						Array damageReturn = healthComp->call("applyDamage", area->_modifiedDamage->Value(), mySource, false, area->_weapon_index);
						if(mySource != nullptr && VariantUtilityFunctions::is_instance_valid(mySource)) {
							Array params;
							params.append(area->DamageCategories);
							params.append(area->_modifiedDamage->Value());
							params.append(damageReturn);
							params.append(hitObj);
							params.append(false);
							mySource->injectEmitSignal("DamageApplied", params);
							if(mySource != area->_gameObject.get_nocheck())
								area->emit_signal("DamageApplied", area->DamageCategories, area->_modifiedDamage->Value(), damageReturn, hitObj, false);
						}
					}
				}
				if(area->ApplyNode.is_valid()) {
					Node* spawnedNode = area->ApplyNode->instantiate();
					hitObj->add_child(spawnedNode);
					if(spawnedNode->has_method("set_modifierSource"))
						spawnedNode->call("set_modifierSource", area->_gameObject.get_nocheck());
				}
			}
		}
	}
}

StatisticsValueData::StatisticsTypes AreaOfEffect::get_display_stats_type() {
	return StatsDisplayType;
}

TypedArray<StatisticsValueData> AreaOfEffect::get_display_stats() {
	if(!DisplayStats) return {};
	TypedArray<StatisticsValueData> allStats;

	if(_modifiedDamage.is_valid())
		allStats.append(
			StatisticsValueData::createFromModifiedValue(
			_modifiedDamage, tr("Damage"), StatsCategory,
			StatisticsValueData::StatisticsFormatTypes::FlatValue,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(_modifiedAttackSpeed.is_valid())
		allStats.append(
			StatisticsValueData::createFromModifiedValue(
			_modifiedAttackSpeed, tr("Attack Speed"), StatsCategory,
			StatisticsValueData::StatisticsFormatTypes::Speed,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(_modifiedArea.is_valid())
		allStats.append(
			StatisticsValueData::createFromModifiedValue(
			_modifiedArea, tr("Area Radius"), StatsCategory,
			StatisticsValueData::StatisticsFormatTypes::Distance,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	return allStats;
}

void AreaOfEffect::initialize_modifiers(Node *referenceParent) {
	if(!RankDamageModifierMethod.is_empty() && GameObject::World() != nullptr) {
		_damageModifier.reference_ptr(Modifier::create("Damage", referenceParent->get("_gameObject")));
		_damageModifier->setModifierName("Torment Scaling");
		_damageModifier->setAdditiveMod(0);
		String value_key;
		if(RankDamageModifierMethod.begins_with("get_"))
			value_key = RankDamageModifierMethod.substr(4);
		else
			value_key = RankDamageModifierMethod;
		_damageModifier->setMultiplierMod(static_cast<float>(StaticValueHelper::get_value(value_key)) - 1);
		_gameObject->triggerModifierUpdated("Damage");
	}
	_modifiedDamage = referenceParent->call("createModifiedFloatValue", ApplyDamage, "Damage");
	float attackSpeed = TriggerEverySeconds == 0 ? 9999.0f : 1.0f / TriggerEverySeconds;
	_modifiedAttackSpeed = referenceParent->call("createModifiedFloatValue", attackSpeed, "AttackSpeed", Callable());
	_modifiedAttackSpeed->connect("ValueUpdated", callable_mp(this, &AreaOfEffect::attackSpeedWasChanged));
	_modifiedArea = referenceParent->call("createModifiedFloatValue", Radius, "Area");
	applyModifierCategories();
}

void AreaOfEffect::applyModifierCategories() {
	_modifiedDamage->setModifierCategories(static_cast<TypedArray<String>>(ModifierCategories));
	_modifiedAttackSpeed->setModifierCategories(static_cast<TypedArray<String>>(ModifierCategories));
	_modifiedArea->setModifierCategories(static_cast<TypedArray<String>>(ModifierCategories));
}

void AreaOfEffect::attackSpeedWasChanged(float oldValue, float newValue) {
	if(newValue == 0) {
		// this is something like a stun. so the emit timer has
		// to be very large (so it doesn't trigger). but we also
		// want to preserve the current value (so that repeated
		// stunning doesn't result in no attacks at all)
		_remainingTimeToNextTrigger = (_remainingTimeToNextTrigger + 100.0f) * 100000.0f;
	}
	else if(oldValue == 0) {
		// coming out of a stun, try to reconstruct the emit timer
		_remainingTimeToNextTrigger = (_remainingTimeToNextTrigger / 100000.0f) - 100.0f;
		// safeguard against strange values:
		if(_remainingTimeToNextTrigger < 0) _remainingTimeToNextTrigger = 0;
		else if(_remainingTimeToNextTrigger > 1.0f / newValue) _remainingTimeToNextTrigger = 1.0f / newValue;
	}
	else {
		// we can apply the proportional change to the
		// currently running time (but: attackSpeed is
		// inverse of time!)
		_remainingTimeToNextTrigger *= oldValue / newValue;
	}
}

void AreaOfEffect::set_harmless(bool harmless) {
	_is_harmless = harmless;
	if(harmless) _remainingTimeToNextTrigger = 99999;
	else _remainingTimeToNextTrigger = TriggerEverySeconds;
}
