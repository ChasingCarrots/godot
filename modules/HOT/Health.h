#ifndef HEALTH_H
#define HEALTH_H

#include "ModifiedValues.h"

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class Health : public GameObjectComponent {
	GDCLASS(Health, GameObjectComponent)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

private:
	// pointer to nodes, that are used regularly
	Node* _world = nullptr;
	Node* _fx = nullptr;
	Node* _global = nullptr;

public:
	enum DamageEffectType {
		None, Burn, Electrify, Frost
	};

	TypedArray<String> ModifierCategories;
	int StartHealth = 100;
	String RankHealthModifierMethod;
	bool DestroyWhenKilled = true;
	float InvincibilityTimeWhenDamaged = 0;
	float InitialInvincibilityTime = 0.2f;
	float BaseChanceToBlock = 0.0f;
	int BaseBlockValue = 0;
	bool InterruptPiercing = false;
	bool ShowDamageNumbers = true;
	bool ShowHealNumbers = false;
	bool AddsToKillCount = false;
	int KillAfterNumberOfHits = -1;
	float BaseDamageFactor = 1;
	int HealOnLevelUp = 0;
	float BaseRegenerationPerSecond = 0;
	int BaseDefense = 0;
	String RankDefenseModifierMethod;

	int _currentHealth;
	TypedArray<float> _invincibilityStacks;
	float _remainingInvincibilityAfterDamage;
	float _damageTakenBeforeInvincibility;
	int _hitCount;
	float _regenBucket;

	Ref<ModifiedIntValue> _modifiedMaxHealth;
	Ref<ModifiedFloatValue> _modifiedChanceToBlock;
	Ref<ModifiedIntValue> _modifiedBlockValue;
	Ref<ModifiedFloatValue> _modifiedDamageFactor;
	Ref<ModifiedFloatValue> _modifiedRegeneration;
	Ref<ModifiedFloatValue> _modifiedDamageFromEffectsFactor;
	Ref<ModifiedIntValue> _modifiedDefense;

	StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
	String StatsCategory = "Defensive Values";

	TypedArray<StatisticsValueData> get_display_stats();

	void applyModifierCategories();

	LocalVector<Callable> _cheatDeathCallableStack;
	void addCheatDeathCallable(Callable callable, bool addToFront);
	void removeCheatDeathCallable(Callable callable);

	void _ready();
	void _world_ready();
	void _exit_tree();
	void _process(float delta);

	void resetToMaxHealth();
	void checkAndUpdateNeedsProcess(float _nop1=0, float _nop2=0);
	void maxHealthWasUpdated(int oldHealth, int newMaxHealth);
	void defenseWasUpdated(int oldDefense, int newDefense);
	bool is_invincible();
	bool is_fullHealth();
	bool is_dead();
	float get_healthPercentage();
	int get_maxHealth();
	int get_blockValue();
	int get_defense();
	float get_regeneration();
	float get_totalChanceToBlock(int damage_amount);
	void add_health(int add, Node* byNode=nullptr);
	void reduce_health(int reduce, bool canKill, Node* source, bool countashit);
	bool needsProcess();
	int apply_defense_to_damage_value(int damageAmount);
	Array applyDamage(int damageAmount, Node* byNode, bool critical, int weapon_index, bool unblockable, DamageEffectType damageEffectType);
	void set_invincible_after_damage();
	float setInvincibleForTime(float invincibleDuration);
	void removeInvincibleForTime(float invincibilityIdentifier);
	void internal_invincibility_started();
	void internal_invincibility_ended();
	float get_totalDamageFactor();
	void on_level_up();
	void instaKill();
	void force_health_update_signals();



	[[nodiscard]] TypedArray<String> GetModifierCategories() const { return ModifierCategories; }
	void SetModifierCategories(const TypedArray<String> &modifierCategories) { ModifierCategories = modifierCategories; }
	[[nodiscard]] int GetStartHealth() const { return StartHealth; }
	void SetStartHealth(int startHealth) { StartHealth = startHealth; }
	[[nodiscard]] String GetRankHealthModifierMethod() const { return RankHealthModifierMethod; }
	void SetRankHealthModifierMethod(const String &rankHealthModifierMethod) { RankHealthModifierMethod = rankHealthModifierMethod; }
	[[nodiscard]] bool GetDestroyWhenKilled() const { return DestroyWhenKilled; }
	void SetDestroyWhenKilled(bool destroyWhenKilled) { DestroyWhenKilled = destroyWhenKilled; }
	[[nodiscard]] float GetInvincibilityTimeWhenDamaged() const { return InvincibilityTimeWhenDamaged; }
	void SetInvincibilityTimeWhenDamaged(float invincibilityTimeWhenDamaged) { InvincibilityTimeWhenDamaged = invincibilityTimeWhenDamaged; }
	[[nodiscard]] float GetInitialInvincibilityTime() const { return InitialInvincibilityTime; }
	void SetInitialInvincibilityTime(float initialInvincibilityTime) { InitialInvincibilityTime = initialInvincibilityTime; }
	[[nodiscard]] float GetBaseChanceToBlock() const { return BaseChanceToBlock; }
	void SetBaseChanceToBlock(float baseChanceToBlock) { BaseChanceToBlock = baseChanceToBlock; }
	[[nodiscard]] int GetBaseBlockValue() const { return BaseBlockValue; }
	void SetBaseBlockValue(int baseBlockValue) { BaseBlockValue = baseBlockValue; }
	[[nodiscard]] bool GetInterruptPiercing() const { return InterruptPiercing; }
	void SetInterruptPiercing(bool interruptPiercing) { InterruptPiercing = interruptPiercing; }
	[[nodiscard]] bool GetShowDamageNumbers() const { return ShowDamageNumbers; }
	void SetShowDamageNumbers(bool showDamageNumbers) { ShowDamageNumbers = showDamageNumbers; }
	[[nodiscard]] bool GetShowHealNumbers() const { return ShowHealNumbers; }
	void SetShowHealNumbers(bool showHealNumbers) { ShowHealNumbers = showHealNumbers; }
	[[nodiscard]] bool GetAddsToKillCount() const { return AddsToKillCount; }
	void SetAddsToKillCount(bool addsToKillCount) { AddsToKillCount = addsToKillCount; }
	[[nodiscard]] int GetKillAfterNumberOfHits() const { return KillAfterNumberOfHits; }
	void SetKillAfterNumberOfHits(int killAfterNumberOfHits) { KillAfterNumberOfHits = killAfterNumberOfHits; }
	[[nodiscard]] float GetBaseDamageFactor() const { return BaseDamageFactor; }
	void SetBaseDamageFactor(float baseDamageFactor) { BaseDamageFactor = baseDamageFactor; }
	[[nodiscard]] int GetHealOnLevelUp() const { return HealOnLevelUp; }
	void SetHealOnLevelUp(int healOnLevelUp) { HealOnLevelUp = healOnLevelUp; }
	[[nodiscard]] float GetBaseRegenerationPerSecond() const { return BaseRegenerationPerSecond; }
	void SetBaseRegenerationPerSecond(float baseRegenerationPerSecond) { BaseRegenerationPerSecond = baseRegenerationPerSecond; }
	[[nodiscard]] int GetBaseDefense() const { return BaseDefense; }
	void SetBaseDefense(int baseDefense) { BaseDefense = baseDefense; }
	[[nodiscard]] String GetRankDefenseModifierMethod() const { return RankDefenseModifierMethod; }
	void SetRankDefenseModifierMethod(const String &rankDefenseModifierMethod) { RankDefenseModifierMethod = rankDefenseModifierMethod; }
	[[nodiscard]] int GetCurrentHealth() const { return _currentHealth; }
	void SetCurrentHealth(int currentHealth) { _currentHealth = currentHealth; }
	[[nodiscard]] TypedArray<float> GetInvincibilityStacks() const { return _invincibilityStacks; }
	void SetInvincibilityStacks(const TypedArray<float> &invincibilityStacks) { _invincibilityStacks = invincibilityStacks; }
	[[nodiscard]] float GetRemainingInvincibilityAfterDamage() const { return _remainingInvincibilityAfterDamage; }
	void SetRemainingInvincibilityAfterDamage(float remainingInvincibilityAfterDamage) { _remainingInvincibilityAfterDamage = remainingInvincibilityAfterDamage; }
	[[nodiscard]] float GetDamageTakenBeforeInvincibility() const { return _damageTakenBeforeInvincibility; }
	void SetDamageTakenBeforeInvincibility(float damageTakenBeforeInvincibility) { _damageTakenBeforeInvincibility = damageTakenBeforeInvincibility; }
	[[nodiscard]] int GetHitCount() const { return _hitCount; }
	void SetHitCount(int hitCount) { _hitCount = hitCount; }
	[[nodiscard]] float GetRegenBucket() const { return _regenBucket; }
	void SetRegenBucket(float regenBucket) { _regenBucket = regenBucket; }
	[[nodiscard]] Ref<ModifiedIntValue> GetModifiedMaxHealth() const { return _modifiedMaxHealth; }
	void SetModifiedMaxHealth(const Ref<ModifiedIntValue> &modifiedMaxHealth) { _modifiedMaxHealth = modifiedMaxHealth; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedChanceToBlock() const { return _modifiedChanceToBlock; }
	void SetModifiedChanceToBlock(const Ref<ModifiedFloatValue> &modifiedChanceToBlock) { _modifiedChanceToBlock = modifiedChanceToBlock; }
	[[nodiscard]] Ref<ModifiedIntValue> GetModifiedBlockValue() const { return _modifiedBlockValue; }
	void SetModifiedBlockValue(const Ref<ModifiedIntValue> &modifiedBlockValue) { _modifiedBlockValue = modifiedBlockValue; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedDamageFactor() const { return _modifiedDamageFactor; }
	void SetModifiedDamageFactor(const Ref<ModifiedFloatValue> &modifiedDamageFactor) { _modifiedDamageFactor = modifiedDamageFactor; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedRegeneration() const { return _modifiedRegeneration; }
	void SetModifiedRegeneration(const Ref<ModifiedFloatValue> &modifiedRegeneration) { _modifiedRegeneration = modifiedRegeneration; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedDamageFromEffectsFactor() const { return _modifiedDamageFromEffectsFactor; }
	void SetModifiedDamageFromEffectsFactor(const Ref<ModifiedFloatValue> &modifiedDamageFromEffectsFactor) { _modifiedDamageFromEffectsFactor = modifiedDamageFromEffectsFactor; }
	[[nodiscard]] Ref<ModifiedIntValue> GetModifiedDefense() const { return _modifiedDefense; }
	void SetModifiedDefense(const Ref<ModifiedIntValue> &modifiedDefense) { _modifiedDefense = modifiedDefense; }
	[[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
	void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
	[[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
	void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
};

VARIANT_ENUM_CAST(Health::DamageEffectType);

#endif //HEALTH_H
