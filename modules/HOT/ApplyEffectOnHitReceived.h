#ifndef APPLYEFFECTONHITRECEIVED_H
#define APPLYEFFECTONHITRECEIVED_H


#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class ApplyEffectOnHitReceived : public GameObjectComponent {
	GDCLASS(ApplyEffectOnHitReceived, GameObjectComponent)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();

  void _notification(int p_notification);
  void _enter_tree();

private:
  Ref<PackedScene> EffectNode;
  float ApplyChance = 1;
  String ChanceModifier;
  TypedArray<StringName> ModifierCategories;
  String CooldownModifier;
  float Cooldown;
  String RangeModifier;
  float ApplyInRadius;
  String RadiusLocatorPool = "Enemies";
  int RemoveGameObjectAfterNumApplies = -1;

  int _numTimesApplied;
  float _nextAllowedTime;
  Ref<ModifiedFloatValue> _modifiedChance;
  Ref<ModifiedFloatValue> _modifiedCooldown;
  Ref<ModifiedFloatValue> _modifiedAreaRadius;

  float get_cooldown() const;
  float get_range() const;
  float get_effect_chance() const;

  StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
  String StatsCategory;
  bool DisplayStats = true;
  TypedArray<StatisticsValueData> get_display_stats();

  // ModifierInfoArea variables
  Ref<Texture2D> Icon;
  String Name;
  String TooltipText;
  Ref<Texture2D> get_modifierInfoArea_icon() const { return Icon; }
  float get_modifierInfoArea_cooldownfactor() const {
    float cd = get_cooldown();
    float remainingTime = _nextAllowedTime - (float)GameObject::World()->get("current_world_time", nullptr);
    if (remainingTime < 0.0f)
      return 0.0f;
    return remainingTime / cd;
  }
  bool get_modifierInfoArea_active() const {
    return _nextAllowedTime <= (float)GameObject::World()->get("current_world_time", nullptr);
  }
  String get_modifierInfoArea_name() const { return Name; }
  String get_modifierInfoArea_tooltip() const { return tr(TooltipText); }
  String get_modifierInfoArea_valuestr() const { return (String)GameObject::Global()->call("formatLargeNumberShortened", _numTimesApplied); }



public:
  // workaround for correct default values of arrays...
  ApplyEffectOnHitReceived() {
    ModifierCategories.append("Physical");
  }
  void initialize_modifiers(Node* referenceParent);
  void applyModifierCategories();
  void onHitTake(bool isInvincible, bool wasBlocked, Node* byNode);

  [[nodiscard]] Ref<PackedScene> GetEffectNode() const { return EffectNode; }
  void SetEffectNode(const Ref<PackedScene> &effectNode) { EffectNode = effectNode; }
  [[nodiscard]] float GetApplyChance() const { return ApplyChance; }
  void SetApplyChance(const float applyChance) { ApplyChance = applyChance; }
  [[nodiscard]] String GetChanceModifier() const { return ChanceModifier; }
  void SetChanceModifier(const String &chanceModifier) { ChanceModifier = chanceModifier; }
  [[nodiscard]] TypedArray<StringName> GetModifierCategories() const { return ModifierCategories; }
  void SetModifierCategories(const TypedArray<StringName> &modifierCategories) { ModifierCategories = modifierCategories; }
  [[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
  void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
  [[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
  void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
  bool GetDisplayStats() const { return DisplayStats; }
  void SetDisplayStats(bool displayStats) { DisplayStats = displayStats; }
  [[nodiscard]] String GetCooldownModifier() const { return CooldownModifier; }
  void SetCooldownModifier(String cooldownModifier) { CooldownModifier = cooldownModifier; }
  [[nodiscard]] float GetCooldown() const { return Cooldown; }
  void SetCooldown(float cooldown) { Cooldown = cooldown; }
  [[nodiscard]] String GetRangeModifier() const { return RangeModifier; }
  void SetRangeModifier(String rangeModifier) { RangeModifier = rangeModifier; }
  [[nodiscard]] float GetApplyInRadius() const { return ApplyInRadius; }
  void SetApplyInRadius(float applyInRadius) { ApplyInRadius = applyInRadius; }
  [[nodiscard]] String GetRadiusLocatorPool() const { return RadiusLocatorPool; }
  void SetRadiusLocatorPool(String radiusLocatorPool) { RadiusLocatorPool = radiusLocatorPool; }
  [[nodiscard]] int GetRemoveGameObjectAfterNumApplies() const { return RemoveGameObjectAfterNumApplies; }
  void SetRemoveGameObjectAfterNumApplies(int removeGameObjectAfterNumApplies) { RemoveGameObjectAfterNumApplies = removeGameObjectAfterNumApplies; }
  [[nodiscard]] Ref<Texture2D> GetIcon() const { return Icon; }
  void SetIcon(Ref<Texture2D> icon) { Icon = icon; }
  [[nodiscard]] String GetName() const { return Name; }
  void SetName(String name) { Name = name; }
  [[nodiscard]] String GetTooltipText() const { return TooltipText; }
  void SetTooltipText(String tooltipText) { TooltipText = tooltipText; }
};



#endif //APPLYEFFECTONHITRECEIVED_H
