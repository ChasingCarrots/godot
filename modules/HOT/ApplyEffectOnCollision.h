#ifndef APPLYEFFECTONCOLLISION_H
#define APPLYEFFECTONCOLLISION_H

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class ApplyEffectOnCollision : public GameObjectComponent {
  GDCLASS(ApplyEffectOnCollision, GameObjectComponent)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();

  void _notification(int p_notification);
  void _enter_tree();

private:
  Ref<PackedScene> EffectNode;
  float ApplyChance = 1;
  String ChanceModifier;
  int RemoveGameObjectAfterNumApplies = -1;
  TypedArray<StringName> ModifierCategories;

  int _numTimesApplied = 0;
  Ref<ModifiedFloatValue> _modifiedChance;
  Ref<ModifiedFloatValue> _modifiedOnHitChance;

  float get_total_on_hit_chance() const;

  StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
  String StatsCategory;
  String EffectName;
  TypedArray<StatisticsValueData> get_display_stats();

public:
  // workaround for correct default values of arrays...
  ApplyEffectOnCollision() {
    ModifierCategories.append("Physical");
  }
  void initialize_modifiers(Node* referenceParent);
  void applyModifierCategories();
  void collisionWithNode(Node* node);


  [[nodiscard]] Ref<PackedScene> GetEffectNode() const { return EffectNode; }
  void SetEffectNode(const Ref<PackedScene> &effectNode) { EffectNode = effectNode; }
  [[nodiscard]] float GetApplyChance() const { return ApplyChance; }
  void SetApplyChance(const float applyChance) { ApplyChance = applyChance; }
  [[nodiscard]] String GetChanceModifier() const { return ChanceModifier; }
  void SetChanceModifier(const String &chanceModifier) { ChanceModifier = chanceModifier; }
  [[nodiscard]] int GetRemoveGameObjectAfterNumApplies() const { return RemoveGameObjectAfterNumApplies; }
  void SetRemoveGameObjectAfterNumApplies(const int removeGameObjectAfterNumApplies) { RemoveGameObjectAfterNumApplies = removeGameObjectAfterNumApplies; }
  [[nodiscard]] String GetEffectName() const { return EffectName; }
  void SetEffectName(const String &effectName) { EffectName = effectName; }
  [[nodiscard]] TypedArray<StringName> GetModifierCategories() const { return ModifierCategories; }
  void SetModifierCategories(const TypedArray<StringName> &modifierCategories) { ModifierCategories = modifierCategories; }
  [[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
  void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
  [[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
  void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
};



#endif //APPLYEFFECTONCOLLISION_H
