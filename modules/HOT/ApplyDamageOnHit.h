#ifndef APPLYDAMAGEONHIT_H
#define APPLYDAMAGEONHIT_H

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class ApplyDamageOnHit : public GameObjectComponent {
  GDCLASS(ApplyDamageOnHit, GameObjectComponent)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();

  void _notification(int p_notification);
  void _enter_tree();
  void _exit_tree();

private:
  int DamageAmount = 0;
  bool Blockable = true;
  float CritChance = 0;
  float CritBonus = 0;
  TypedArray<StringName> ModifierCategories;
  TypedArray<String> DamageCategories;
  int ApplyOnSignal = 0;
  int MaxNumberOfHits = 999999;
  float DamageMultPerHit = 1.0;
  float DamageChangePerSec = 0;
  bool IsCharacterBaseNode = false;
  int BonusStat = 0;
  TypedArray<String> BonusTriggerEffects;
  float BonusAmount = 0;
  bool BonusIsFlat = true;
  String QuestID;
  bool ReportCritChance = false;
  int _weapon_index = -1;

  StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
  String StatsCategory;
  bool HideDamage = false;
  TypedArray<StatisticsValueData> get_display_stats();

	Ref<ModifiedIntValue> _modifiedDamage;
	Ref<ModifiedFloatValue> _modifiedForce;
	Ref<ModifiedFloatValue> _modifiedCriticalHitChance;
	Ref<ModifiedFloatValue> _modifiedCriticalHitBonus;
	float _time_started = 0;
	int _numberOfHits = 0;
	void registerCritChanceReporter();
public:
  // workaround for default values of Arrays
  ApplyDamageOnHit() {
		ModifierCategories.append("Physical");
	}

	float get_bonus_value(float original_value, GameObject* targetNode, int bonusType);
	float get_totalCritChance();
	float get_totalCritBonus();
	float get_totalDamageChangePerSecond();
	float get_damageReductionMultiplier();

	void initialize_modifiers(Node* referenceParent);
	void applyModifierCategories();
	void transformCategories(const String& onlyWithDamageCategory,
	    const TypedArray<StringName>& addModifierCategories, const TypedArray<StringName>& removeModifierCategories,
	    const TypedArray<StringName>& addDamageCategories, const TypedArray<StringName>& removeDamageCategories);
	void _on_critchance_update(float oldValue, float newValue);
	void _on_hit(GameObject* node, int hitNumber);
	void resetTimeDamageModifier();
	void collisionWithNode(Node* otherNode);

	[[nodiscard]] int GetDamageAmount() const { return DamageAmount; }
	void SetDamageAmount(int damageAmount) { DamageAmount = damageAmount; }
	[[nodiscard]] bool GetBlockable() const { return Blockable; }
	void SetBlockable(bool blockable) { Blockable = blockable; }
	[[nodiscard]] float GetCritChance() const { return CritChance; }
	void SetCritChance(float critChance) { CritChance = critChance; }
	[[nodiscard]] float GetCritBonus() const { return CritBonus; }
	void SetCritBonus(float critBonus) { CritBonus = critBonus; }
	[[nodiscard]] TypedArray<StringName> GetModifierCategories() const { return ModifierCategories; }
	void SetModifierCategories(const TypedArray<StringName> &modifierCategories) { ModifierCategories = modifierCategories; }
	[[nodiscard]] TypedArray<String> GetDamageCategories() const { return DamageCategories; }
	void SetDamageCategories(const TypedArray<String> &damageCategories) { DamageCategories = damageCategories; }
	[[nodiscard]] int GetApplyOnSignal() const { return ApplyOnSignal; }
	void SetApplyOnSignal(int applyOnSignal) { ApplyOnSignal = applyOnSignal; }
	[[nodiscard]] int GetMaxNumberOfHits() const { return MaxNumberOfHits; }
	void SetMaxNumberOfHits(int maxNumberOfHits) { MaxNumberOfHits = maxNumberOfHits; }
	[[nodiscard]] float GetDamageMultPerHit() const { return DamageMultPerHit; }
	void SetDamageMultPerHit(float damageMultPerHit) { DamageMultPerHit = damageMultPerHit; }
	[[nodiscard]] float GetDamageChangePerSec() const { return DamageChangePerSec; }
	void SetDamageChangePerSec(float damageChangePerSec) { DamageChangePerSec = damageChangePerSec; }
	[[nodiscard]] bool GetIsCharacterBaseNode() const { return IsCharacterBaseNode; }
	void SetIsCharacterBaseNode(bool isCharacterBaseNode) { IsCharacterBaseNode = isCharacterBaseNode; }
	[[nodiscard]] int GetBonusStat() const { return BonusStat; }
	void SetBonusStat(int bonusStat) { BonusStat = bonusStat; }
	[[nodiscard]] TypedArray<String> GetBonusTriggerEffects() const { return BonusTriggerEffects; }
	void SetBonusTriggerEffects(const TypedArray<String> &bonusTriggerEffects) { BonusTriggerEffects = bonusTriggerEffects; }
	[[nodiscard]] float GetBonusAmount() const { return BonusAmount; }
	void SetBonusAmount(float bonusAmount) { BonusAmount = bonusAmount; }
	[[nodiscard]] bool GetBonusIsFlat() const { return BonusIsFlat; }
	void SetBonusIsFlat(bool bonusIsFlat) { BonusIsFlat = bonusIsFlat; }
	[[nodiscard]] String GetQuestID() const { return QuestID; }
	void SetQuestID(const String &questId) { QuestID = questId; }
	[[nodiscard]] bool GetReportCritChance() const { return ReportCritChance; }
	void SetReportCritChance(bool reportCritChance) { ReportCritChance = reportCritChance; }
	[[nodiscard]] int GetWeaponIndex() const { return _weapon_index; }
	void SetWeaponIndex(int weaponIndex) { _weapon_index = weaponIndex; }
	[[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
	void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
	[[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
	void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
	[[nodiscard]] bool GetHideDamage() const { return HideDamage; }
	void SetHideDamage(bool hideDamage) { HideDamage = hideDamage; }
};



#endif //APPLYDAMAGEONHIT_H
