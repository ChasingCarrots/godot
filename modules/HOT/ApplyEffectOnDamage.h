#ifndef APPLYEFFECTONDAMAGE_H
#define APPLYEFFECTONDAMAGE_H

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class ApplyEffectOnDamage : public GameObjectComponent {
	GDCLASS(ApplyEffectOnDamage, GameObjectComponent)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();

  void _notification(int p_notification);
  void _ready();

private:
  Ref<PackedScene> EffectNode;
  float ApplyChance = 1;
  String ChanceModifier;
  float MultiplyChanceEveryTimeChecked = 1.0f;
  bool ApplyOnlyOnCritical = false;
  bool ApplyOnlyWhenActuallyDamaged = true;
  TypedArray<StringName> ModifierCategories;
  PackedStringArray OnlyForDamageCategories = {"DefaultWeapon"};
  bool TrackMaxStackQuest = false;
  int TrackID = -1;
  float ChanceForSelfApply = 0;
  Ref<PackedScene> SelfApplyNode;

  Ref<ModifiedFloatValue> _modifiedChance;
  Ref<ModifiedFloatValue> _modifiedOnHitChance;
  Ref<ModifiedFloatValue> _modifiedOnCritChance;
  int _effect_weapon_index = -1;
  bool _effect_has_was_killed = false;

  float get_total_on_hit_chance() const;
  float get_total_on_crit_chance() const;

  StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
  String StatsCategory;
  String EffectName;
  TypedArray<StatisticsValueData> get_display_stats();

public:
  // workaround for correct default values of arrays...
  ApplyEffectOnDamage() {
    ModifierCategories.append("Physical");
  }
  void initialize_modifiers(Node* referenceParent);
  void transformCategories(const String& onlyWithModifierCategory,
    const TypedArray<StringName>& addModifierCategories, const TypedArray<StringName>& removeModifierCategories,
    const TypedArray<StringName>& addDamageCategories, const TypedArray<StringName>& removeDamageCategories);
  void applyModifierCategories();
  void damageWasApplied(const TypedArray<String>& categories, float damageAmount, Array applyReturn, GameObject* targetNode, bool isCritical);
  void externalDamageWasApplied(const TypedArray<String>& categories, float damageAmount, Array applyReturn, GameObject* targetNode, bool isCritical);

  [[nodiscard]] Ref<PackedScene> GetEffectNode() const { return EffectNode; }
  void SetEffectNode(const Ref<PackedScene> &effectNode) { EffectNode = effectNode; }
  [[nodiscard]] float GetApplyChance() const { return ApplyChance; }
  void SetApplyChance(const float applyChance) { ApplyChance = applyChance; }
  [[nodiscard]] String GetChanceModifier() const { return ChanceModifier; }
  void SetChanceModifier(const String &chanceModifier) { ChanceModifier = chanceModifier; }
  [[nodiscard]] String GetEffectName() const { return EffectName; }
  void SetEffectName(const String &effectName) { EffectName = effectName; }
  float GetMultiplyChanceEveryTimeChecked() const { return MultiplyChanceEveryTimeChecked; }
  void SetMultiplyChanceEveryTimeChecked(float multiplyChanceEveryTimeChecked) { MultiplyChanceEveryTimeChecked = multiplyChanceEveryTimeChecked; }
  bool GetApplyOnlyOnCritical() const { return ApplyOnlyOnCritical; }
  void SetApplyOnlyOnCritical(bool applyOnlyOnCritical) { ApplyOnlyOnCritical = applyOnlyOnCritical; }
  bool GetApplyOnlyWhenActuallyDamaged() const { return ApplyOnlyWhenActuallyDamaged; }
  void SetApplyOnlyWhenActuallyDamaged(bool applyOnlyWhenActuallyDamaged) { ApplyOnlyWhenActuallyDamaged = applyOnlyWhenActuallyDamaged; }
  PackedStringArray GetOnlyForDamageCategories() const { return OnlyForDamageCategories; }
  void SetOnlyForDamageCategories(PackedStringArray onlyForDamageCategories) { OnlyForDamageCategories = onlyForDamageCategories; }
  bool GetTrackMaxStackQuest() const { return TrackMaxStackQuest; }
  void SetTrackMaxStackQuest(bool trackMaxStackQuest) { TrackMaxStackQuest = trackMaxStackQuest; }
  int GetTrackID() const { return TrackID; }
  void SetTrackID(int trackId) { TrackID = trackId; }
  float GetChanceForSelfApply() const { return ChanceForSelfApply; }
  void SetChanceForSelfApply(float chanceForSelfApply) { ChanceForSelfApply = chanceForSelfApply; }
  Ref<PackedScene> GetSelfApplyNode() const { return SelfApplyNode; }
  void SetSelfApplyNode(Ref<PackedScene> selfApplyNode) { SelfApplyNode = selfApplyNode; }
  [[nodiscard]] TypedArray<StringName> GetModifierCategories() const { return ModifierCategories; }
  void SetModifierCategories(const TypedArray<StringName> &modifierCategories) { ModifierCategories = modifierCategories; }
  [[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
  void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
  [[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
  void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
};



#endif //APPLYEFFECTONDAMAGE_H
