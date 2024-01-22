#include "Health.h"

#include <core/variant/variant_utility.h>
#include <scene/main/window.h>
#include <core/math/math_funcs.h>
#include "Statistics.h"

const Color CRIT_COLOR = Color(1.0, 0.4, 0.3);
const Color BURN_COLOR = Color(1.0, 0.7, 0.0);
const Color ELECTRIFY_COLOR = Color(0.4, 0.7, 1.0);
const Color FROST_COLOR = Color(0.5, 0.5, 0.9);

const float DEF_BASE_COEFF = 40.0f;
const float DEF_BASE_INV_COEFF = 1.0f / DEF_BASE_COEFF;
const float DEF_FACTOR_COEFF = 0.4f;

void Health::_bind_methods() {
	BIND_ENUM_CONSTANT(None);
	BIND_ENUM_CONSTANT(Burn);
	BIND_ENUM_CONSTANT(Electrify);
	BIND_ENUM_CONSTANT(Frost);
	
	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &Health::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &Health::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_ModifierCategories", "get_ModifierCategories");
	ClassDB::bind_method(D_METHOD("set_StartHealth", "startHealth"), &Health::SetStartHealth);
	ClassDB::bind_method(D_METHOD("get_StartHealth"), &Health::GetStartHealth);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StartHealth"), "set_StartHealth", "get_StartHealth");
	ClassDB::bind_method(D_METHOD("set_RankHealthModifierMethod", "rankHealthModifierMethod"), &Health::SetRankHealthModifierMethod);
	ClassDB::bind_method(D_METHOD("get_RankHealthModifierMethod"), &Health::GetRankHealthModifierMethod);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "RankHealthModifierMethod"), "set_RankHealthModifierMethod", "get_RankHealthModifierMethod");
	ClassDB::bind_method(D_METHOD("set_DestroyWhenKilled", "destroyWhenKilled"), &Health::SetDestroyWhenKilled);
	ClassDB::bind_method(D_METHOD("get_DestroyWhenKilled"), &Health::GetDestroyWhenKilled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "DestroyWhenKilled"), "set_DestroyWhenKilled", "get_DestroyWhenKilled");
	ClassDB::bind_method(D_METHOD("set_InvincibilityTimeWhenDamaged", "invincibilityTimeWhenDamaged"), &Health::SetInvincibilityTimeWhenDamaged);
	ClassDB::bind_method(D_METHOD("get_InvincibilityTimeWhenDamaged"), &Health::GetInvincibilityTimeWhenDamaged);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "InvincibilityTimeWhenDamaged"), "set_InvincibilityTimeWhenDamaged", "get_InvincibilityTimeWhenDamaged");
	ClassDB::bind_method(D_METHOD("set_InitialInvincibilityTime", "initialInvincibilityTime"), &Health::SetInitialInvincibilityTime);
	ClassDB::bind_method(D_METHOD("get_InitialInvincibilityTime"), &Health::GetInitialInvincibilityTime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "InitialInvincibilityTime"), "set_InitialInvincibilityTime", "get_InitialInvincibilityTime");
	ClassDB::bind_method(D_METHOD("set_BaseChanceToBlock", "baseChanceToBlock"), &Health::SetBaseChanceToBlock);
	ClassDB::bind_method(D_METHOD("get_BaseChanceToBlock"), &Health::GetBaseChanceToBlock);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BaseChanceToBlock"), "set_BaseChanceToBlock", "get_BaseChanceToBlock");
	ClassDB::bind_method(D_METHOD("set_BaseBlockValue", "baseBlockValue"), &Health::SetBaseBlockValue);
	ClassDB::bind_method(D_METHOD("get_BaseBlockValue"), &Health::GetBaseBlockValue);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "BaseBlockValue"), "set_BaseBlockValue", "get_BaseBlockValue");
	ClassDB::bind_method(D_METHOD("set_InterruptPiercing", "interruptPiercing"), &Health::SetInterruptPiercing);
	ClassDB::bind_method(D_METHOD("get_InterruptPiercing"), &Health::GetInterruptPiercing);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "InterruptPiercing"), "set_InterruptPiercing", "get_InterruptPiercing");
	ClassDB::bind_method(D_METHOD("set_ShowDamageNumbers", "showDamageNumbers"), &Health::SetShowDamageNumbers);
	ClassDB::bind_method(D_METHOD("get_ShowDamageNumbers"), &Health::GetShowDamageNumbers);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ShowDamageNumbers"), "set_ShowDamageNumbers", "get_ShowDamageNumbers");
	ClassDB::bind_method(D_METHOD("set_ShowHealNumbers", "showHealNumbers"), &Health::SetShowHealNumbers);
	ClassDB::bind_method(D_METHOD("get_ShowHealNumbers"), &Health::GetShowHealNumbers);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ShowHealNumbers"), "set_ShowHealNumbers", "get_ShowHealNumbers");
	ClassDB::bind_method(D_METHOD("set_AddsToKillCount", "addsToKillCount"), &Health::SetAddsToKillCount);
	ClassDB::bind_method(D_METHOD("get_AddsToKillCount"), &Health::GetAddsToKillCount);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "AddsToKillCount"), "set_AddsToKillCount", "get_AddsToKillCount");
	ClassDB::bind_method(D_METHOD("set_KillAfterNumberOfHits", "killAfterNumberOfHits"), &Health::SetKillAfterNumberOfHits);
	ClassDB::bind_method(D_METHOD("get_KillAfterNumberOfHits"), &Health::GetKillAfterNumberOfHits);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "KillAfterNumberOfHits"), "set_KillAfterNumberOfHits", "get_KillAfterNumberOfHits");
	ClassDB::bind_method(D_METHOD("set_BaseDamageFactor", "baseDamageFactor"), &Health::SetBaseDamageFactor);
	ClassDB::bind_method(D_METHOD("get_BaseDamageFactor"), &Health::GetBaseDamageFactor);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BaseDamageFactor"), "set_BaseDamageFactor", "get_BaseDamageFactor");
	ClassDB::bind_method(D_METHOD("set_HealOnLevelUp", "healOnLevelUp"), &Health::SetHealOnLevelUp);
	ClassDB::bind_method(D_METHOD("get_HealOnLevelUp"), &Health::GetHealOnLevelUp);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "HealOnLevelUp"), "set_HealOnLevelUp", "get_HealOnLevelUp");
	ClassDB::bind_method(D_METHOD("set_BaseRegenerationPerSecond", "baseRegenerationPerSecond"), &Health::SetBaseRegenerationPerSecond);
	ClassDB::bind_method(D_METHOD("get_BaseRegenerationPerSecond"), &Health::GetBaseRegenerationPerSecond);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BaseRegenerationPerSecond"), "set_BaseRegenerationPerSecond", "get_BaseRegenerationPerSecond");
	ClassDB::bind_method(D_METHOD("set_BaseDefense", "baseDefense"), &Health::SetBaseDefense);
	ClassDB::bind_method(D_METHOD("get_BaseDefense"), &Health::GetBaseDefense);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "BaseDefense"), "set_BaseDefense", "get_BaseDefense");
	ClassDB::bind_method(D_METHOD("set_RankDefenseModifierMethod", "rankDefenseModifierMethod"), &Health::SetRankDefenseModifierMethod);
	ClassDB::bind_method(D_METHOD("get_RankDefenseModifierMethod"), &Health::GetRankDefenseModifierMethod);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "RankDefenseModifierMethod"), "set_RankDefenseModifierMethod", "get_RankDefenseModifierMethod");

	// the following properties should not show up in the inspector (PROPERTY_USAGE_NONE)
	ClassDB::bind_method(D_METHOD("set_CurrentHealth", "_currentHealth"), &Health::SetCurrentHealth);
	ClassDB::bind_method(D_METHOD("get_CurrentHealth"), &Health::GetCurrentHealth);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_currentHealth", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_CurrentHealth", "get_CurrentHealth");
	ClassDB::bind_method(D_METHOD("set_InvincibilityStacks", "_invincibilityStacks"), &Health::SetInvincibilityStacks);
	ClassDB::bind_method(D_METHOD("get_InvincibilityStacks"), &Health::GetInvincibilityStacks);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "_invincibilityStacks", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_InvincibilityStacks", "get_InvincibilityStacks");
	ClassDB::bind_method(D_METHOD("set_RemainingInvincibilityAfterDamage", "_remainingInvincibilityAfterDamage"), &Health::SetRemainingInvincibilityAfterDamage);
	ClassDB::bind_method(D_METHOD("get_RemainingInvincibilityAfterDamage"), &Health::GetRemainingInvincibilityAfterDamage);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_remainingInvincibilityAfterDamage", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_RemainingInvincibilityAfterDamage", "get_RemainingInvincibilityAfterDamage");
	ClassDB::bind_method(D_METHOD("set_DamageTakenBeforeInvincibility", "_damageTakenBeforeInvincibility"), &Health::SetDamageTakenBeforeInvincibility);
	ClassDB::bind_method(D_METHOD("get_DamageTakenBeforeInvincibility"), &Health::GetDamageTakenBeforeInvincibility);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_damageTakenBeforeInvincibility", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_DamageTakenBeforeInvincibility", "get_DamageTakenBeforeInvincibility");
	ClassDB::bind_method(D_METHOD("set_HitCount", "_hitCount"), &Health::SetHitCount);
	ClassDB::bind_method(D_METHOD("get_HitCount"), &Health::GetHitCount);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_hitCount", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_HitCount", "get_HitCount");
	ClassDB::bind_method(D_METHOD("set_RegenBucket", "_regenBucket"), &Health::SetRegenBucket);
	ClassDB::bind_method(D_METHOD("get_RegenBucket"), &Health::GetRegenBucket);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_regenBucket", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_RegenBucket", "get_RegenBucket");
	ClassDB::bind_method(D_METHOD("set_ModifiedMaxHealth", "_modifiedMaxHealth"), &Health::SetModifiedMaxHealth);
	ClassDB::bind_method(D_METHOD("get_ModifiedMaxHealth"), &Health::GetModifiedMaxHealth);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedMaxHealth", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedMaxHealth", "get_ModifiedMaxHealth");
	ClassDB::bind_method(D_METHOD("set_ModifiedChanceToBlock", "_modifiedChanceToBlock"), &Health::SetModifiedChanceToBlock);
	ClassDB::bind_method(D_METHOD("get_ModifiedChanceToBlock"), &Health::GetModifiedChanceToBlock);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedChanceToBlock", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedChanceToBlock", "get_ModifiedChanceToBlock");
	ClassDB::bind_method(D_METHOD("set_ModifiedBlockValue", "_modifiedBlockValue"), &Health::SetModifiedBlockValue);
	ClassDB::bind_method(D_METHOD("get_ModifiedBlockValue"), &Health::GetModifiedBlockValue);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedBlockValue", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedBlockValue", "get_ModifiedBlockValue");
	ClassDB::bind_method(D_METHOD("set_ModifiedDamageFactor", "_modifiedDamageFactor"), &Health::SetModifiedDamageFactor);
	ClassDB::bind_method(D_METHOD("get_ModifiedDamageFactor"), &Health::GetModifiedDamageFactor);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedDamageFactor", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedDamageFactor", "get_ModifiedDamageFactor");
	ClassDB::bind_method(D_METHOD("set_ModifiedRegeneration", "_modifiedRegeneration"), &Health::SetModifiedRegeneration);
	ClassDB::bind_method(D_METHOD("get_ModifiedRegeneration"), &Health::GetModifiedRegeneration);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedRegeneration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedRegeneration", "get_ModifiedRegeneration");
	ClassDB::bind_method(D_METHOD("set_ModifiedDamageFromEffectsFactor", "_modifiedDamageFromEffectsFactor"), &Health::SetModifiedDamageFromEffectsFactor);
	ClassDB::bind_method(D_METHOD("get_ModifiedDamageFromEffectsFactor"), &Health::GetModifiedDamageFromEffectsFactor);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedDamageFromEffectsFactor", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedDamageFromEffectsFactor", "get_ModifiedDamageFromEffectsFactor");
	ClassDB::bind_method(D_METHOD("set_ModifiedDefense", "_modifiedDefense"), &Health::SetModifiedDefense);
	ClassDB::bind_method(D_METHOD("get_ModifiedDefense"), &Health::GetModifiedDefense);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedDefense", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedDefense", "get_ModifiedDefense");

	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &Health::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &Health::GetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats"), &Health::get_display_stats);
	ClassDB::bind_method(D_METHOD("addCheatDeathCallable", "callable", "addToFront"), &Health::addCheatDeathCallable, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("removeCheatDeathCallable", "callable"), &Health::removeCheatDeathCallable);
	ClassDB::bind_method(D_METHOD("resetToMaxHealth"), &Health::resetToMaxHealth);
	ClassDB::bind_method(D_METHOD("is_invincible"), &Health::is_invincible);
	ClassDB::bind_method(D_METHOD("is_fullHealth"), &Health::is_fullHealth);
	ClassDB::bind_method(D_METHOD("is_dead"), &Health::is_dead);
	ClassDB::bind_method(D_METHOD("get_blockValue"), &Health::get_blockValue);
	ClassDB::bind_method(D_METHOD("get_defense"), &Health::get_defense);
	ClassDB::bind_method(D_METHOD("get_regeneration"), &Health::get_regeneration);
	ClassDB::bind_method(D_METHOD("get_totalChanceToBlock"), &Health::get_totalChanceToBlock);
	ClassDB::bind_method(D_METHOD("add_health", "add", "byNode"), &Health::add_health, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("reduce_health", "reduce", "cankill", "source", "countashit"), &Health::reduce_health, DEFVAL(false), DEFVAL(Variant()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("applyDamage", "damageAmount", "byNode", "critical", "weapon_index", "unblockable", "damageEffectType"), &Health::applyDamage, DEFVAL(false), DEFVAL(-1), DEFVAL(false), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("setInvincibleForTime", "invincibleDuration"), &Health::setInvincibleForTime);
	ClassDB::bind_method(D_METHOD("removeInvincibleForTime", "invincibilityIdentifier"), &Health::removeInvincibleForTime);
	ClassDB::bind_method(D_METHOD("instakill"), &Health::instaKill);
	ClassDB::bind_method(D_METHOD("force_health_update_signals"), &Health::force_health_update_signals);


	// these already exist as getter (above), but can just be bound additionally to different names...
	ClassDB::bind_method(D_METHOD("get_health"), &Health::GetCurrentHealth);
	ClassDB::bind_method(D_METHOD("get_maxHealth"), &Health::get_maxHealth);
	ClassDB::bind_method(D_METHOD("get_maxHealthBase"), &Health::GetStartHealth);
	ClassDB::bind_method(D_METHOD("get_baseChanceToBlock"), &Health::GetBaseChanceToBlock);
	ClassDB::bind_method(D_METHOD("get_baseBaseBlockValue"), &Health::GetBaseBlockValue);

	ADD_SIGNAL(MethodInfo("Killed", PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("ReceivedDamage", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node"), PropertyInfo(Variant::INT, "weapon_index")));
	ADD_SIGNAL(MethodInfo("OnInvincibilityStateChanged", PropertyInfo(Variant::BOOL, "isInvincible")));
	ADD_SIGNAL(MethodInfo("HealthChanged", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::INT, "change")));
	ADD_SIGNAL(MethodInfo("DefenseChanged", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::INT, "change")));
	ADD_SIGNAL(MethodInfo("MaxHealthChanged", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::INT, "change")));
	ADD_SIGNAL(MethodInfo("Regeneration", PropertyInfo(Variant::INT, "amount")));
	ADD_SIGNAL(MethodInfo("BlockedDamage", PropertyInfo(Variant::INT, "amount")));
	ADD_SIGNAL(MethodInfo("OnHitTaken", PropertyInfo(Variant::BOOL, "invincible"), PropertyInfo(Variant::BOOL, "blocked"), PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("InvincibilityNegatedDamage", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("DeathCheated"));
	ADD_SIGNAL(MethodInfo("Instakilled", PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("OnHealed", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::OBJECT, "byNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));


}

void Health::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
			break;
		case NOTIFICATION_PROCESS:
			_process(get_process_delta_time());
			break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
			break;
	}
}

TypedArray<StatisticsValueData> Health::get_display_stats() {
	TypedArray<StatisticsValueData> stats;
	stats.append(StatisticsValueData::createFromModifiedValue(
		_modifiedMaxHealth, tr("Max Health"), StatsCategory,
		StatisticsValueData::StatisticsFormatTypes::FlatValue, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	stats.append(StatisticsValueData::createFromModifiedValue(
		_modifiedRegeneration, tr("Health Regen"), StatsCategory,
		StatisticsValueData::StatisticsFormatTypes::Speed, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	stats.append(StatisticsValueData::createFromModifiedValue(
		_modifiedBlockValue, tr("Block Strength"), StatsCategory,
		StatisticsValueData::StatisticsFormatTypes::FlatValue, StatisticsValueData::StatisticsSpecialInfoTypes::Block));
	stats.append(StatisticsValueData::createFromModifiedValue(
		_modifiedDefense, tr("Defense"), StatsCategory,
		StatisticsValueData::StatisticsFormatTypes::FlatValue, StatisticsValueData::StatisticsSpecialInfoTypes::Defense));
	return stats;
}

void Health::applyModifierCategories() {
	if(_modifiedMaxHealth != nullptr)
		_modifiedMaxHealth->setModifierCategories(ModifierCategories);
	if(_modifiedChanceToBlock != nullptr)
		_modifiedChanceToBlock->setModifierCategories(ModifierCategories);
	if(_modifiedBlockValue != nullptr)
		_modifiedBlockValue->setModifierCategories(ModifierCategories);
	if(_modifiedDamageFactor != nullptr)
		_modifiedDamageFactor->setModifierCategories(ModifierCategories);
	if(_modifiedRegeneration != nullptr)
		_modifiedRegeneration->setModifierCategories(ModifierCategories);
	if(_modifiedDamageFromEffectsFactor != nullptr)
		_modifiedDamageFromEffectsFactor->setModifierCategories(ModifierCategories);
	if(_modifiedDefense != nullptr)
		_modifiedDefense->setModifierCategories(ModifierCategories);
}

void Health::addCheatDeathCallable(Callable callable, bool addToFront) {
	int i = _cheatDeathCallableStack.find(callable);
	if(i == -1) {
		if(addToFront) _cheatDeathCallableStack.insert(0, callable);
		else _cheatDeathCallableStack.push_back(callable);
	}
}

void Health::removeCheatDeathCallable(Callable callable) {
	_cheatDeathCallableStack.erase(callable);
}

void Health::_ready() {
	PROFILE_FUNCTION();
	init_gameobject_component();
	_remainingInvincibilityAfterDamage = InitialInvincibilityTime;
	_damageTakenBeforeInvincibility = 9999999;
	_modifiedChanceToBlock = create_modified_float_value(BaseChanceToBlock, "BlockChance");
	_modifiedBlockValue = create_modified_int_value(BaseBlockValue, "BlockValue");
	_modifiedDamageFactor = create_modified_float_value(BaseDamageFactor, "DamageFactor");
	_modifiedDamageFromEffectsFactor = create_modified_float_value(BaseDamageFactor, "DamageFromEffectsFactor");

	_fx = get_tree()->get_root()->get_node(NodePath("Fx"));
	if (_fx == nullptr)
		print_error("Fx autoload node not found!");

	_global = get_tree()->get_root()->get_node(NodePath("Global"));
	if (_global == nullptr) {
		print_error("Global autoload node not found!");
		return;
	}

	if (RankHealthModifierMethod.is_empty())
		_modifiedMaxHealth = create_modified_int_value(StartHealth, "MaxHealth");
	if (RankDefenseModifierMethod.is_empty())
		_modifiedDefense = create_modified_int_value(BaseDefense, "Defense");

	if (_global->call("is_world_ready"))
		_world_ready();
	else
		_global->connect("WorldReady", callable_mp(this, &Health::_world_ready));

	_modifiedRegeneration = create_modified_float_value(BaseRegenerationPerSecond, "HealthRegen");
	_modifiedRegeneration->connect("ValueUpdated", callable_mp(this, &Health::checkAndUpdateNeedsProcess));
	_modifiedMaxHealth->connect("ValueUpdated", callable_mp(this, &Health::maxHealthWasUpdated));
	_modifiedDefense->connect("ValueUpdated", callable_mp(this, &Health::defenseWasUpdated));
	resetToMaxHealth();
	checkAndUpdateNeedsProcess();
	applyModifierCategories();
	set_process(true);
}


void Health::_world_ready() {
	Variant worldVariant = _global->get("World");
	if (worldVariant.is_null()) {
		print_error("Global.World is null!");
		return;
	}
	_world = cast_to<Node>(worldVariant);
	Variant tormentRank = _world->get("TormentRank");
	if (tormentRank.is_null()) {
		print_error("World.TormentRank is null!");
		return;
	}
	_world->connect("ExperienceThresholdReached", callable_mp(this, &Health::on_level_up));

	if (!RankHealthModifierMethod.is_empty() && tormentRank.has_method(RankHealthModifierMethod))
		_modifiedMaxHealth = create_modified_int_value(
				StartHealth * (float)tormentRank.call(RankHealthModifierMethod), "MaxHealth");
	else if(_modifiedMaxHealth.is_null())
		_modifiedMaxHealth = create_modified_int_value(StartHealth, "MaxHealth");

	if (!RankDefenseModifierMethod.is_empty() && tormentRank.has_method(RankDefenseModifierMethod))
		_modifiedDefense = create_modified_int_value(
				BaseDefense + (float)tormentRank.call(RankDefenseModifierMethod), "Defense");
	else if(_modifiedDefense.is_null())
		_modifiedDefense = create_modified_int_value(BaseDefense, "Defense");
}

void Health::_exit_tree() {
	_modifiedMaxHealth = nullptr;
	_modifiedChanceToBlock = nullptr;
	_modifiedBlockValue = nullptr;
	_modifiedDamageFactor = nullptr;
	_modifiedRegeneration = nullptr;
	_modifiedDamageFromEffectsFactor = nullptr;
	_modifiedDefense = nullptr;
}

void Health::_process(float delta) {
	PROFILE_FUNCTION();
	if(get_regeneration() != 0.0) {
		_regenBucket += delta * get_regeneration();
		if(_regenBucket >= 1.0) {
			if(_currentHealth < get_maxHealth()) {
				add_health(static_cast<int>(floor(_regenBucket)));
				emit_signal("Regeneration", static_cast<int>(floor(_regenBucket)));
			}
			_regenBucket = fmod(_regenBucket, 1.0f);
		}
		else if(_regenBucket <= -1.0) {
			if(_currentHealth > 1)
				reduce_health(int(floor(abs(_regenBucket))), false, nullptr, false);
			_regenBucket = fmod(_regenBucket, 1.0f);
		}
	}
	bool invincible_before = is_invincible();
	if(_remainingInvincibilityAfterDamage > 0) {
		_remainingInvincibilityAfterDamage -= delta;
		if(_remainingInvincibilityAfterDamage <= 0)
			_remainingInvincibilityAfterDamage = 0;
	}
	while(!_invincibilityStacks.is_empty() && (float)_invincibilityStacks[0] <= (float)(_world->get("current_world_time")))
		_invincibilityStacks.pop_front();

	if(invincible_before && !is_invincible())
		internal_invincibility_ended();

	checkAndUpdateNeedsProcess();
}

void Health::resetToMaxHealth() {
	_currentHealth = get_maxHealth();
}

void Health::checkAndUpdateNeedsProcess(float _nop1, float _nop2) {
	if(needsProcess()) set_process_mode(PROCESS_MODE_PAUSABLE);
	else set_process_mode(PROCESS_MODE_DISABLED);
}

void Health::maxHealthWasUpdated(int oldHealth, int newMaxHealth) {
	int healthDelta = 0;
	if(_currentHealth > newMaxHealth) {
		healthDelta = newMaxHealth - _currentHealth;
		_currentHealth = std::max(1, newMaxHealth);
	}
	// we do not add to the current health here (the modifier does
	// that, when necessary), but we still have to emit the signal!
	emit_signal("MaxHealthChanged", newMaxHealth, newMaxHealth-oldHealth);
	emit_signal("HealthChanged", _currentHealth, healthDelta);
}

void Health::defenseWasUpdated(int oldDefense, int newDefense) {
	int defenseDelta = newDefense - oldDefense;
	emit_signal("DefenseChanged", newDefense, defenseDelta);
}

bool Health::is_invincible() {
	return _remainingInvincibilityAfterDamage > 0.0 || !_invincibilityStacks.is_empty();
}

bool Health::is_fullHealth() {
	return _currentHealth == get_maxHealth();
}

bool Health::is_dead() {
	return _currentHealth <= 0;
}

float Health::get_healthPercentage() {
	return static_cast<float>(_currentHealth) / static_cast<float>(get_maxHealth());
}

int Health::get_maxHealth() {
	return std::max(1, _modifiedMaxHealth->Value());
}

int Health::get_blockValue() {
	return _modifiedBlockValue->Value();
}

int Health::get_defense() {
	return _modifiedDefense->Value();
}

float Health::get_regeneration() {
	return _modifiedRegeneration->Value();
}

float Health::get_totalChanceToBlock(int damage_amount) {
	float block_fraction = static_cast<float>(_modifiedBlockValue->Value()) / static_cast<float>(damage_amount);
	return std::min(1.0f, std::min(block_fraction * 0.5f, pow(block_fraction, 0.5f) * 0.5f) + _modifiedChanceToBlock->Value());
}

void Health::add_health(int add, Node *byNode) {
	PROFILE_FUNCTION();
	if(add < 0) {
		print_error("add_health only supports positive numbers, use applyDamage instead...");
		return;
	}
	int actualAdd = add;
	if(_currentHealth + add > get_maxHealth()) {
		actualAdd = get_maxHealth() - _currentHealth;
		_currentHealth = get_maxHealth();
	}
	else {
		_currentHealth += add;
	}

	if(actualAdd > 0)
		emit_signal("HealthChanged", _currentHealth, actualAdd);
	emit_signal("OnHealed", actualAdd, byNode);

	if(ShowHealNumbers && _positionProvider.is_valid()) {
		_fx->call("show_text_indicator",
			get_gameobject_worldposition() + Vector2(0,-1) * 20.0f,
			String::num_int64(add), 2, 1.5f, Color(0, 1, 0, 1));
	}
}

void Health::reduce_health(int reduce, bool canKill, Node *source, bool countashit) {
	PROFILE_FUNCTION();
	if(reduce < 0) {
		print_error("reduce_health only supports positive numbers, use applyDamage instead...");
		return;
	}
	int actualReduce = reduce;
	if(_currentHealth - reduce < 0 && !canKill)
		actualReduce = _currentHealth - 1;
	// killed part
	if(_currentHealth <= actualReduce) {
		_currentHealth = 0;
		emit_signal("OnHitTaken", false, false, source);
		emit_signal("ReceivedDamage", actualReduce, source, -1);
		emit_signal("HealthChanged", _currentHealth, -actualReduce);
		if(_cheatDeathCallableStack.size() > 0) {
			Callable currentDeathCheatCallable = _cheatDeathCallableStack[_cheatDeathCallableStack.size()-1];
			currentDeathCheatCallable.call();
			emit_signal("DeathCheated");
			return;
		}
		emit_signal("Killed", source);
		if(AddsToKillCount) {
			_world->call("TriggerDamageEvent",
				_gameObject.get(),
				source,
				actualReduce,
				actualReduce,
				false,
				-1);
			_world->call("TriggerDeathEvent", _gameObject.get(), source);
		}
	}
	else if(actualReduce > 0) {
		_currentHealth -= reduce;
		emit_signal("HealthChanged", _currentHealth, -actualReduce);
		if(countashit) {
			emit_signal("OnHitTaken", false, false, source);
			emit_signal("ReceivedDamage", actualReduce, source, -1);
		}
	}
}

bool Health::needsProcess() {
	return _remainingInvincibilityAfterDamage > 0 || get_regeneration() != 0 || !_invincibilityStacks.is_empty();
}

int Health::apply_defense_to_damage_value(int damageAmount) {
	int def = _modifiedDefense->Value();
	float part_1 = 1.0f / ((abs(def) + DEF_BASE_COEFF) * DEF_BASE_INV_COEFF);
	if(def < 0)
		part_1 = 2-part_1;
	part_1 = part_1 * (1.0 - DEF_FACTOR_COEFF);
	float part_2 = std::max(0.0f, (1.0f - (def / 100.0f)) * DEF_FACTOR_COEFF);
	return std::max(1, static_cast<int>(round(damageAmount * (part_1 + part_2))));
}

// IMPORTANT: this enum should always be kept up to date with the one in Global.gd
// it is replicated here for convenience and performance...
enum class ApplyDamageResult {
	Invalid,
	Blocked,
	Invincible,
	DamagedButNotKilled,
	Killed,
	CheatedDeath
};

Array Health::applyDamage(int damageAmount, Node *byNode, bool critical, int weapon_index, bool unblockable, DamageEffectType damageEffectType) {
	PROFILE_FUNCTION();
	Array retValue;
	if(is_invincible() && damageAmount <= _damageTakenBeforeInvincibility) {
		emit_signal("InvincibilityNegatedDamage", damageAmount, byNode);
		emit_signal("OnHitTaken", true, false, byNode);
		retValue.append(static_cast<int>(ApplyDamageResult::Invincible));
		retValue.append(0);
		return retValue;
	}
	if(damageAmount < 0) {
		print_error("the damage amount in applyDamage has to be a positive value!");
		retValue.append(static_cast<int>(ApplyDamageResult::Invalid));
		retValue.append(0);
		return retValue;
	}
	if(_currentHealth <= 0) {
		retValue.append(static_cast<int>(ApplyDamageResult::Invalid));
		retValue.append(0);
		return retValue;
	}

	if(!unblockable && Math::randf() <= get_totalChanceToBlock(damageAmount)) {
		if(InvincibilityTimeWhenDamaged > 0)
			_damageTakenBeforeInvincibility = damageAmount;
		if(_positionProvider.is_valid()) {
			_fx->call("show_text_indicator",
				get_gameobject_worldposition() + Vector2(0,-1) * 32.0f, "", 1, 2.0f);
			_fx->call("show_block", get_gameobject_worldposition() + Vector2(0,-1) * 16.0f);
		}
		emit_signal("OnHitTaken", false, true, byNode);
		emit_signal("BlockedDamage", damageAmount);
		set_invincible_after_damage();
		retValue.append(static_cast<int>(ApplyDamageResult::Blocked));
		retValue.append(0);
		return retValue;
	}

	int scaledDamageAmount = damageAmount;
	if(damageEffectType == DamageEffectType::None)
		scaledDamageAmount = Math::round(damageAmount * get_totalDamageFactor());
	else
		scaledDamageAmount = Math::round(damageAmount * _modifiedDamageFromEffectsFactor->Value());
	scaledDamageAmount = apply_defense_to_damage_value(scaledDamageAmount);
	if(ShowDamageNumbers && _positionProvider.is_valid()) {
		if(critical)
			_fx->call("show_text_indicator",
				get_gameobject_worldposition() + Vector2(0,-1) * 20.0f,
				String::num_int64(scaledDamageAmount), 0, 2.0, CRIT_COLOR);
		else {
			if(damageEffectType == DamageEffectType::Burn)
				_fx->call("show_text_indicator",
					get_gameobject_worldposition() + Vector2(0,-1) * 20.0,
					String::num_int64(scaledDamageAmount), 4, 1.0, BURN_COLOR);
			else if(damageEffectType == DamageEffectType::Electrify)
				_fx->call("show_text_indicator",
					get_gameobject_worldposition() + Vector2(0,-1) * 20.0,
					String::num_int64(scaledDamageAmount), 3, 1.0, ELECTRIFY_COLOR);
			else if(damageEffectType == DamageEffectType::Frost)
				_fx->call("show_text_indicator",
					get_gameobject_worldposition() + Vector2(0,-1) * 20.0,
					String::num_int64(scaledDamageAmount), 6, 1.0, FROST_COLOR);
			else
				_fx->call("show_text_indicator",
					get_gameobject_worldposition() + Vector2(0,-1) * 20.0,
					String::num_int64(scaledDamageAmount));
		}
	}

	// We want incoming damage to pierce through invincibility
	// when it's higher then the damage that triggered the current invincibility
	// state. If this case applies we want only the remaining damage to be received.
	// Reason: enemies which deal more damage should take precedence before
	// weaker enemies. This is our way of dealing with this.
	int actualDamageAmount = std::min(std::max(scaledDamageAmount - _damageTakenBeforeInvincibility, 0.0f), 9999999.0f);
	if(InvincibilityTimeWhenDamaged > 0)
		_damageTakenBeforeInvincibility = damageAmount;

	_hitCount += 1;

	bool killedByLackOfHealth = actualDamageAmount >= _currentHealth;
	bool killedByHitCount = KillAfterNumberOfHits > 0 && _hitCount >= KillAfterNumberOfHits;
	if(killedByLackOfHealth || killedByHitCount) {
		int overkillDamage = std::max(0, actualDamageAmount - _currentHealth);
		if(killedByLackOfHealth)
			actualDamageAmount = _currentHealth;
		if(killedByHitCount)
			// we can't really name a sensible actualDamage amount when
			// the object simply gets destroyed after a certain amount of
			// hits. (example: breakables. they have a huge amount of health)
			actualDamageAmount = 0;
		_currentHealth = 0;
		emit_signal("OnHitTaken", false, false, byNode);
		emit_signal("ReceivedDamage", actualDamageAmount, byNode, weapon_index);
		emit_signal("HealthChanged", _currentHealth, -actualDamageAmount);
		if(_cheatDeathCallableStack.size() > 0) {
			Callable currentDeathCheatCallable = _cheatDeathCallableStack[_cheatDeathCallableStack.size()-1];
			currentDeathCheatCallable.call();
			emit_signal("DeathCheated");
			retValue.append(static_cast<int>(ApplyDamageResult::CheatedDeath));
			retValue.append(0);
			return retValue;
		}
		emit_signal("Killed", byNode);
		if(actualDamageAmount == get_maxHealth()) {
			Variant questPool = _global->get("QuestPool");
			questPool.call("notify_one_hit_kill", actualDamageAmount, weapon_index);
		}
		if(AddsToKillCount) {
			_world->call("TriggerDamageEvent",
				_gameObject.get(),
				byNode,
				actualDamageAmount,
				scaledDamageAmount,
				critical,
				weapon_index);
			_world->call("TriggerDeathEvent", _gameObject.get(), byNode);
		}

		Stats::AddDamageEvent(static_cast<GameObject *>(byNode), _gameObject.get(), actualDamageAmount, weapon_index);

		if(DestroyWhenKilled && _gameObject.is_valid())
			_gameObject->queue_free();
		retValue.append(static_cast<int>(ApplyDamageResult::Killed));
		retValue.append(actualDamageAmount);
		retValue.append(overkillDamage);
		return retValue;
	}
	set_invincible_after_damage();

	_currentHealth -= actualDamageAmount;
	emit_signal("OnHitTaken", false, false, byNode);
	emit_signal("ReceivedDamage", actualDamageAmount, byNode, weapon_index);
	if(AddsToKillCount)
		_world->call("TriggerDamageEvent",
			_gameObject.get(),
			byNode,
			actualDamageAmount,
			scaledDamageAmount,
			critical,
			weapon_index);
	emit_signal("HealthChanged", _currentHealth, -actualDamageAmount);
	Stats::AddDamageEvent(static_cast<GameObject *>(byNode), _gameObject.get(), actualDamageAmount, weapon_index);
	if(InterruptPiercing) {
		retValue.append(static_cast<int>(ApplyDamageResult::Blocked));
		retValue.append(actualDamageAmount);
		return retValue;
	}
	retValue.append(static_cast<int>(ApplyDamageResult::DamagedButNotKilled));
	retValue.append(actualDamageAmount);
	return retValue;
}

void Health::set_invincible_after_damage() {
	if(InvincibilityTimeWhenDamaged > 0) {
		bool invincible_before = is_invincible();
		_remainingInvincibilityAfterDamage = InvincibilityTimeWhenDamaged;

		if(!invincible_before) {
			checkAndUpdateNeedsProcess();
			// don't use internal_invincibility_started() here, because we
			// actually don't want to set the damageTakenBeforeInvincibility
			// when it is a damage event that lead to the invincibility!
			emit_signal("OnInvincibilityStateChanged", true);
		}
	}
}

float Health::setInvincibleForTime(float invincibleDuration) {
	bool invincible_before = is_invincible();
	float invincibility_end_time = static_cast<float>(_world->get("current_world_time")) + invincibleDuration;
	_invincibilityStacks.append(invincibility_end_time);
	_invincibilityStacks.sort();
	if(!invincible_before) {
		checkAndUpdateNeedsProcess();
		internal_invincibility_started();
	}
	return invincibility_end_time;
}

void Health::removeInvincibleForTime(float invincibilityIdentifier) {
	bool invincible_before = is_invincible();
	_invincibilityStacks.erase(invincibilityIdentifier);
	if(invincible_before && ! is_invincible()) {
		checkAndUpdateNeedsProcess();
		internal_invincibility_ended();
	}
}

void Health::internal_invincibility_started() {
	_damageTakenBeforeInvincibility = 99999;
	emit_signal("OnInvincibilityStateChanged", true);
}

void Health::internal_invincibility_ended() {
	_damageTakenBeforeInvincibility = 0;
	emit_signal("OnInvincibilityStateChanged", false);
}

float Health::get_totalDamageFactor() {
	return _modifiedDamageFactor->Value();
}

void Health::on_level_up() {
	if(HealOnLevelUp > 0)
		add_health(HealOnLevelUp);
}

void Health::instaKill() {
	emit_signal("Instakilled", Variant());
	if(_gameObject.is_valid())
		_gameObject->queue_free();
}

void Health::force_health_update_signals() {
	emit_signal("MaxHealthChanged", get_maxHealth(), 0);
	emit_signal("HealthChanged", GetCurrentHealth(), 0);
}
