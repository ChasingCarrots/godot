#include "ApplyDamageOnHit.h"

#include "Health.h"

#include <core/variant/variant_utility.h>

void ApplyDamageOnHit::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_DamageAmount", "damageAmount"), &ApplyDamageOnHit::SetDamageAmount);
	ClassDB::bind_method(D_METHOD("get_DamageAmount"), &ApplyDamageOnHit::GetDamageAmount);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "DamageAmount"), "set_DamageAmount", "get_DamageAmount");

	ClassDB::bind_method(D_METHOD("set_Blockable", "blockable"), &ApplyDamageOnHit::SetBlockable);
	ClassDB::bind_method(D_METHOD("get_Blockable"), &ApplyDamageOnHit::GetBlockable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "Blockable"), "set_Blockable", "get_Blockable");

	ClassDB::bind_method(D_METHOD("set_CritChance", "critChance"), &ApplyDamageOnHit::SetCritChance);
	ClassDB::bind_method(D_METHOD("get_CritChance"), &ApplyDamageOnHit::GetCritChance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "CritChance"), "set_CritChance", "get_CritChance");

	ClassDB::bind_method(D_METHOD("set_CritBonus", "critBonus"), &ApplyDamageOnHit::SetCritBonus);
	ClassDB::bind_method(D_METHOD("get_CritBonus"), &ApplyDamageOnHit::GetCritBonus);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "CritBonus"), "set_CritBonus", "get_CritBonus");

	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &ApplyDamageOnHit::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &ApplyDamageOnHit::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"), "set_ModifierCategories", "get_ModifierCategories");
	TypedArray<StringName> defaultModifierCategories;
	defaultModifierCategories.append("Physical");
	ADD_PROPERTY_DEFAULT("ModifierCategories", defaultModifierCategories);

	ClassDB::bind_method(D_METHOD("set_DamageCategories", "damageCategories"), &ApplyDamageOnHit::SetDamageCategories);
	ClassDB::bind_method(D_METHOD("get_DamageCategories"), &ApplyDamageOnHit::GetDamageCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "DamageCategories", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING) + ":"), "set_DamageCategories", "get_DamageCategories");

	ClassDB::bind_method(D_METHOD("set_ApplyOnSignal", "applyOnSignal"), &ApplyDamageOnHit::SetApplyOnSignal);
	ClassDB::bind_method(D_METHOD("get_ApplyOnSignal"), &ApplyDamageOnHit::GetApplyOnSignal);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "ApplyOnSignal", PROPERTY_HINT_ENUM, "OnHit,CollisionStarted"), "set_ApplyOnSignal", "get_ApplyOnSignal");

	ClassDB::bind_method(D_METHOD("set_MaxNumberOfHits", "maxNumberOfHits"), &ApplyDamageOnHit::SetMaxNumberOfHits);
	ClassDB::bind_method(D_METHOD("get_MaxNumberOfHits"), &ApplyDamageOnHit::GetMaxNumberOfHits);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "MaxNumberOfHits"), "set_MaxNumberOfHits", "get_MaxNumberOfHits");

	ClassDB::bind_method(D_METHOD("set_DamageMultPerHit", "damageMultPerHit"), &ApplyDamageOnHit::SetDamageMultPerHit);
	ClassDB::bind_method(D_METHOD("get_DamageMultPerHit"), &ApplyDamageOnHit::GetDamageMultPerHit);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "DamageMultPerHit"), "set_DamageMultPerHit", "get_DamageMultPerHit");

	ClassDB::bind_method(D_METHOD("set_DamageChangePerSec", "damageChangePerSec"), &ApplyDamageOnHit::SetDamageChangePerSec);
	ClassDB::bind_method(D_METHOD("get_DamageChangePerSec"), &ApplyDamageOnHit::GetDamageChangePerSec);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "DamageChangePerSec"), "set_DamageChangePerSec", "get_DamageChangePerSec");

	ClassDB::bind_method(D_METHOD("set_IsCharacterBaseNode", "isCharacterBaseNode"), &ApplyDamageOnHit::SetIsCharacterBaseNode);
	ClassDB::bind_method(D_METHOD("get_IsCharacterBaseNode"), &ApplyDamageOnHit::GetIsCharacterBaseNode);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "IsCharacterBaseNode"), "set_IsCharacterBaseNode", "get_IsCharacterBaseNode");

	ClassDB::bind_method(D_METHOD("set_BonusStat", "bonusStat"), &ApplyDamageOnHit::SetBonusStat);
	ClassDB::bind_method(D_METHOD("get_BonusStat"), &ApplyDamageOnHit::GetBonusStat);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "BonusStat", PROPERTY_HINT_ENUM, "None,Damage,CritChance,CritBonus"), "set_BonusStat", "get_BonusStat");

	ClassDB::bind_method(D_METHOD("set_BonusTriggerEffects", "bonusTriggerEffects"), &ApplyDamageOnHit::SetBonusTriggerEffects);
	ClassDB::bind_method(D_METHOD("get_BonusTriggerEffects"), &ApplyDamageOnHit::GetBonusTriggerEffects);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "BonusTriggerEffects", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING) + ":"), "set_BonusTriggerEffects", "get_BonusTriggerEffects");

	ClassDB::bind_method(D_METHOD("set_BonusAmount", "bonusAmount"), &ApplyDamageOnHit::SetBonusAmount);
	ClassDB::bind_method(D_METHOD("get_BonusAmount"), &ApplyDamageOnHit::GetBonusAmount);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BonusAmount"), "set_BonusAmount", "get_BonusAmount");
	ADD_PROPERTY_DEFAULT("BonusAmount", 0.0f);

	ClassDB::bind_method(D_METHOD("set_BonusIsFlat", "bonusIsFlat"), &ApplyDamageOnHit::SetBonusIsFlat);
	ClassDB::bind_method(D_METHOD("get_BonusIsFlat"), &ApplyDamageOnHit::GetBonusIsFlat);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "BonusIsFlat"), "set_BonusIsFlat", "get_BonusIsFlat");

	ClassDB::bind_method(D_METHOD("set_QuestID", "questID"), &ApplyDamageOnHit::SetQuestID);
	ClassDB::bind_method(D_METHOD("get_QuestID"), &ApplyDamageOnHit::GetQuestID);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "QuestID"), "set_QuestID", "get_QuestID");

	ClassDB::bind_method(D_METHOD("set_ReportCritChance", "reportCritChance"), &ApplyDamageOnHit::SetReportCritChance);
	ClassDB::bind_method(D_METHOD("get_ReportCritChance"), &ApplyDamageOnHit::GetReportCritChance);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ReportCritChance"), "set_ReportCritChance", "get_ReportCritChance");
	ADD_PROPERTY_DEFAULT("ReportCritChance", false);

	ClassDB::bind_method(D_METHOD("set_weapon_index", "_weapon_index"), &ApplyDamageOnHit::SetWeaponIndex);
	ClassDB::bind_method(D_METHOD("get_weapon_index"), &ApplyDamageOnHit::GetWeaponIndex);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_weapon_index"), "set_weapon_index", "get_weapon_index");

	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &ApplyDamageOnHit::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &ApplyDamageOnHit::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats"), "set_StatsDisplayType", "get_display_stats_type");

	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &ApplyDamageOnHit::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &ApplyDamageOnHit::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");

	ClassDB::bind_method(D_METHOD("set_HideDamage", "hideDamage"), &ApplyDamageOnHit::SetHideDamage);
	ClassDB::bind_method(D_METHOD("get_HideDamage"), &ApplyDamageOnHit::GetHideDamage);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "HideDamage"), "set_HideDamage", "get_HideDamage");

	ClassDB::bind_method(D_METHOD("get_display_stats"), &ApplyDamageOnHit::get_display_stats);
	ClassDB::bind_method(D_METHOD("get_bonus_value", "original_value", "targetNode", "bonusType"), &ApplyDamageOnHit::get_bonus_value);
	ClassDB::bind_method(D_METHOD("get_totalCritChance"), &ApplyDamageOnHit::get_totalCritChance);
	ClassDB::bind_method(D_METHOD("get_totalCritBonus"), &ApplyDamageOnHit::get_totalCritBonus);
	ClassDB::bind_method(D_METHOD("get_totalDamageChangePerSecond"), &ApplyDamageOnHit::get_totalDamageChangePerSecond);
	ClassDB::bind_method(D_METHOD("get_damageReductionMultiplier"), &ApplyDamageOnHit::get_damageReductionMultiplier);
	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &ApplyDamageOnHit::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &ApplyDamageOnHit::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("transformCategories", "onlyWithDamageCategory", "addModifierCategories", "removeModifierCategories", "addDamageCategories", "removeDamageCategories"), &ApplyDamageOnHit::transformCategories);
	ClassDB::bind_method(D_METHOD("_on_critchance_update", "oldValue", "newValue"), &ApplyDamageOnHit::_on_critchance_update);
	ClassDB::bind_method(D_METHOD("_on_hit", "node", "hitNumber"), &ApplyDamageOnHit::_on_hit);
	ClassDB::bind_method(D_METHOD("resetTimeDamageModifier"), &ApplyDamageOnHit::resetTimeDamageModifier);
	ClassDB::bind_method(D_METHOD("collisionWithNode", "otherNode"), &ApplyDamageOnHit::collisionWithNode);

	ADD_SIGNAL(MethodInfo("DamageApplied", PropertyInfo(Variant::ARRAY, "damageCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), PropertyInfo(Variant::FLOAT, "damageAmount"), PropertyInfo(Variant::ARRAY, "applyReturn"), PropertyInfo(Variant::OBJECT, "targetNode", PROPERTY_HINT_RESOURCE_TYPE, "GameObject"), PropertyInfo(Variant::BOOL, "critical")));
}

void ApplyDamageOnHit::_notification(int p_notification) {
	switch (p_notification) {
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

void ApplyDamageOnHit::_enter_tree() {
	init_gameobject_component();
	if(!_gameObject.is_valid())
		return;

	if(ApplyOnSignal == 0)
		_gameObject->connectToSignal("OnHit", callable_mp(this, &ApplyDamageOnHit::_on_hit));
	else if(ApplyOnSignal == 1)
		_gameObject->connectToSignal("CollisionStarted", callable_mp(this, &ApplyDamageOnHit::collisionWithNode));

	initialize_modifiers(this);
	_time_started = (float)GameObject::World()->get("current_world_time");
}

void ApplyDamageOnHit::_exit_tree() {
	if(_gameObject.is_valid() && _numberOfHits > 0)
		GameObject::Global()->get("QuestPool").call("notify_hits_per_projectile", _numberOfHits, _weapon_index);
}

TypedArray<StatisticsValueData> ApplyDamageOnHit::get_display_stats() {
	TypedArray<StatisticsValueData> collectedStats;

	if(BonusStat > 0) {
		static const String BonusStatNames[] = {"None", "Damage", "Crit Chance", "Crit Bonus"};
		String statName = vformat(tr("%s per effect stack"), tr(BonusStatNames[BonusStat]));
		if(BonusTriggerEffects.size() == 1)
			statName = vformat(tr("%s per %s stack"), tr(BonusStatNames[BonusStat]), GameObject::Global()->call("EffectIDToString", BonusTriggerEffects[0]));
		auto FormatType = StatisticsValueData::StatisticsFormatTypes::Percentage;
		if(BonusIsFlat) {
			statName = vformat(tr("Flat %s"), statName);
			if(BonusStat == 1) FormatType = StatisticsValueData::StatisticsFormatTypes::FlatValue;
		}
		collectedStats.append(StatisticsValueData::createNew(
			statName, StatsCategory, "",
			BonusAmount, BonusAmount, ModifierCategories,
			FormatType,	StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}

	if(_modifiedDamage.is_valid() && !HideDamage)
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedDamage, tr("Damage"), StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::FlatValue,
				StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(_modifiedCriticalHitChance.is_valid() && _modifiedCriticalHitChance->Value() > 0.0)
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedCriticalHitChance, tr("Crit Chance"), StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::Percentage,
				StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(_modifiedCriticalHitBonus.is_valid() && _modifiedCriticalHitBonus->Value() > 0.0)
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedCriticalHitBonus, tr("Crit Bonus"), StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::Percentage,
				StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(DamageMultPerHit != 1.0 && _modifiedForce.is_valid())
		collectedStats.append(StatisticsValueData::createNew(
			tr("Damage Mod per Hit (Force)"), StatsCategory, "Force",
			DamageMultPerHit, pow(DamageMultPerHit, get_damageReductionMultiplier()), ModifierCategories,
			StatisticsValueData::StatisticsFormatTypes::Multiplier,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	if(DamageChangePerSec < 0 && _modifiedForce.is_valid())
		collectedStats.append(StatisticsValueData::createNew(
			tr("Damage Reduction per Second (Force)"), StatsCategory, "Force",
			DamageChangePerSec, get_totalDamageChangePerSecond(), ModifierCategories,
			StatisticsValueData::StatisticsFormatTypes::Percentage,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	else if(DamageChangePerSec > 0 && _modifiedForce.is_valid())
		collectedStats.append(StatisticsValueData::createNew(
			tr("Damage Bonus per Second"), StatsCategory, "",
			DamageChangePerSec, get_totalDamageChangePerSecond(), ModifierCategories,
			StatisticsValueData::StatisticsFormatTypes::Percentage,
			StatisticsValueData::StatisticsSpecialInfoTypes::None));

	return collectedStats;
}

float ApplyDamageOnHit::get_bonus_value(float original_value, GameObject *targetNode, int bonusType) {
	if(bonusType != BonusStat)
		return original_value;
	float total_bonus = 0.0f;
	for(const auto BonusTriggerEffect :BonusTriggerEffects) {
		Node* targetEffect = targetNode->find_effect(BonusTriggerEffect);
		if(targetEffect == nullptr)
			continue;
		bool has_num_stacks = false;
		int target_num_stacks = targetEffect->get("_num_stacks", &has_num_stacks);
		if(!has_num_stacks)
			continue;
		if(BonusIsFlat)
			total_bonus += target_num_stacks * BonusAmount;
		else
			total_bonus += original_value * target_num_stacks * BonusAmount;
	}
	return original_value + total_bonus;
}

float ApplyDamageOnHit::get_totalCritChance() {
	if(_modifiedCriticalHitChance.is_valid()) return _modifiedCriticalHitChance->Value();
	return 0;
}

float ApplyDamageOnHit::get_totalCritBonus() {
	if(_modifiedCriticalHitBonus.is_valid()) return _modifiedCriticalHitBonus->Value();
	return 0;
}

float ApplyDamageOnHit::get_totalDamageChangePerSecond() {
	if(DamageChangePerSec < 0.0f && _modifiedForce.is_valid()) return DamageChangePerSec / _modifiedForce->Value();
	return DamageChangePerSec;
}

float ApplyDamageOnHit::get_damageReductionMultiplier() {
	if(_modifiedForce.is_valid()) return 1.0f / _modifiedForce->Value();
	return 1;
}

void ApplyDamageOnHit::initialize_modifiers(Node *referenceParent) {
	GameObject *referenceGameObject = nullptr;
	if (referenceParent == this)
		referenceGameObject = _gameObject.get();
	else
		referenceGameObject = Object::cast_to<GameObject>(referenceParent->get("_gameObject"));
	_modifiedForce.instantiate();
	_modifiedDamage.instantiate();
	_modifiedCriticalHitChance.instantiate();
	_modifiedCriticalHitBonus.instantiate();

	if (referenceGameObject != nullptr) {
		_modifiedForce->_initAsMultiplicativeOnly("Force", referenceGameObject, "");
		_modifiedDamage->_init(DamageAmount, "Damage", referenceGameObject, "");
		_modifiedCriticalHitChance->_init(CritChance, "CritChance", referenceGameObject, "");
		_modifiedCriticalHitBonus->_init(CritBonus, "CritBonus", referenceGameObject, "");
	}

	applyModifierCategories();
	if(ReportCritChance)
		GameObject::Global()->connect("WorldReady", callable_mp(this, &ApplyDamageOnHit::registerCritChanceReporter), ConnectFlags::CONNECT_ONE_SHOT);
}

void ApplyDamageOnHit::registerCritChanceReporter() {
	if(!GameObject::World()->get("Tags").call("isTagActive", QuestID))
		_modifiedCriticalHitChance->connect("ValueUpdated", callable_mp(this, &ApplyDamageOnHit::_on_critchance_update));
}

void ApplyDamageOnHit::applyModifierCategories() {
	if(_modifiedDamage.is_valid()) _modifiedDamage->setModifierCategories(ModifierCategories);
	if(_modifiedForce.is_valid()) _modifiedForce->setModifierCategories(ModifierCategories);
	if(_modifiedCriticalHitChance.is_valid()) _modifiedCriticalHitChance->setModifierCategories(ModifierCategories);
	if(_modifiedCriticalHitBonus.is_valid()) _modifiedCriticalHitBonus->setModifierCategories(ModifierCategories);
}

void ApplyDamageOnHit::transformCategories(const String &onlyWithDamageCategory, const TypedArray<StringName> &addModifierCategories, const TypedArray<StringName> &removeModifierCategories, const TypedArray<StringName> &addDamageCategories, const TypedArray<StringName> &removeDamageCategories) {
	if(!DamageCategories.has(onlyWithDamageCategory))
		return;
	for(const auto& addMod : addModifierCategories) {
		if(!ModifierCategories.has(addMod))
			ModifierCategories.append(addMod);
	}
	for(const auto &remMod : removeModifierCategories) {
		ModifierCategories.erase(remMod);
	}
	for(const auto& addCat : addDamageCategories) {
		if(!DamageCategories.has(addCat))
			DamageCategories.append(addCat);
	}
	for(const auto& removeCat : removeDamageCategories) {
		DamageCategories.erase(removeCat);
	}
	applyModifierCategories();
}

void ApplyDamageOnHit::_on_critchance_update(float oldValue, float newValue) {
	GameObject::Global()->get("QuestPool").call("notify_critchance_update", QuestID, newValue * 100);
}

void ApplyDamageOnHit::_on_hit(GameObject *node, int hitNumber) {
	if(_numberOfHits >= MaxNumberOfHits)
		return;
	Health* healthComponent = dynamic_cast<Health*>(node->getChildNodeWithMethod("applyDamage"));
	if(healthComponent != nullptr) {
		int base_damage = VariantUtilityFunctions::floori(_modifiedDamage->Value());
		int crits = VariantUtilityFunctions::ceili(get_bonus_value(get_totalCritChance(), node, 2) - VariantUtilityFunctions::randf());
		int damage = base_damage + VariantUtilityFunctions::floori(base_damage * crits
			* get_bonus_value(get_totalCritBonus(), node, 3));
		bool critical = damage > base_damage;
		if(damage <= 0)
			return;

		GameObject::Global()->get("QuestPool").call("notify_player_critcount", crits, _weapon_index);

		if(hitNumber > 0 && DamageMultPerHit != 1) {
			float hitMultDamage = float(damage) * pow(DamageMultPerHit, hitNumber*get_damageReductionMultiplier());
			if(hitMultDamage < 1) {
				// when the damage would dip below 1, we remove the bullet,
				// since it can't do any damage anymore!
				_gameObject->queue_free();
				return;
			}
			damage = VariantUtilityFunctions::floori(hitMultDamage);
		}
		if(DamageChangePerSec != 0) {
			float current_world_time = (float)GameObject::World()->get("current_world_time");
			float activeTime = current_world_time - _time_started;
			float timeModeDamage = float(damage) + float(damage) * get_totalDamageChangePerSecond() * activeTime;
			damage = VariantUtilityFunctions::ceili(std::max(1.0f, timeModeDamage));
		}
		damage = VariantUtilityFunctions::floori(get_bonus_value(damage, node, 1));
		GameObject* mySource = _gameObject->get_rootSourceGameObject();
		auto damageReturn = healthComponent->applyDamage(damage, mySource, critical, _weapon_index, !Blockable, Health::None);

		if(mySource != nullptr && VariantUtilityFunctions::is_instance_valid(mySource)) {
			Array params;
			params.append(DamageCategories);
			params.append(damage);
			params.append(damageReturn);
			params.append(node);
			params.append(critical);
			mySource->injectEmitSignal("DamageApplied", params);
			if(mySource != _gameObject.get())
				emit_signal("DamageApplied", DamageCategories, damage, damageReturn, node, critical);
		}
		GameObject::Global()->get("QuestPool").call("notify_player_dealt_uncapped_damage", damage, _weapon_index);
		_numberOfHits += 1;
	}
}

void ApplyDamageOnHit::resetTimeDamageModifier() {
	_time_started  = (float)GameObject::World()->get("current_world_time");
}

void ApplyDamageOnHit::collisionWithNode(Node *otherNode) {
	GameObject* other = Object::cast_to<GameObject>(otherNode);
	if(other != nullptr)
		_on_hit(other, 1);
}
