#ifndef AREAOFEFFECT_H
#define AREAOFEFFECT_H

#include "GameObjectComponents.h"
#include "LocatorSystem.h"
#include "StatisticsValueData.h"

#include <scene/resources/packed_scene.h>
#include <scene/main/window.h>

class AreaOfEffect  : public GameObjectComponent2D {
	GDCLASS(AreaOfEffect, GameObjectComponent2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _exit_tree();
	void _enter_tree();
	void _ready();
	void _notification(int p_notification);

	static LocalVector<AreaOfEffect*> _allAreaOfEffects;

	int ApplyDamage;
	String RankDamageModifierMethod;
	Ref<PackedScene> ApplyNode;
	bool TriggerHitOnChildren = true;
	bool TriggerHitOnParentGameObject = false;
	String LocatorPoolName = "Player";
	float Radius = 30;
	float TriggerEverySeconds = 0;
	float ProbabilityToApply = 1;
	PackedStringArray ModifierCategories = { "Summon" };
	PackedStringArray DamageCategories;
	bool UseModifiedArea = false;

	float _remainingTimeToNextTrigger = 0;
	bool _is_harmless = false;
	Ref<ModifiedFloatValue> _modifiedDamage;
	Ref<Modifier> _damageModifier;
	Ref<ModifiedFloatValue> _modifiedAttackSpeed;
	Ref<ModifiedFloatValue> _modifiedArea;
	int _weapon_index = -1;

	StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::StatisticsTypes::PlayerBaseStats;
	String StatsCategory;
	bool DisplayStats = false;

public:
	static void UpdateAllAreaOfEffects(float delta);

	StatisticsValueData::StatisticsTypes get_display_stats_type();
	TypedArray<StatisticsValueData> get_display_stats();

	void initialize_modifiers(Node* referenceParent);
	void applyModifierCategories();
	void attackSpeedWasChanged(float oldvalue, float newvalue);
	void set_harmless(bool harmless);

	[[nodiscard]] int GetApplyDamage() const { return ApplyDamage; }
	void SetApplyDamage(int applyDamage) { ApplyDamage = applyDamage; }
	[[nodiscard]] String GetRankDamageModifierMethod() const { return RankDamageModifierMethod; }
	void SetRankDamageModifierMethod(const String &rankDamageModifierMethod) { RankDamageModifierMethod = rankDamageModifierMethod; }
	[[nodiscard]] Ref<PackedScene> GetApplyNode() const { return ApplyNode; }
	void SetApplyNode(const Ref<PackedScene> &applyNode) { ApplyNode = applyNode; }
	[[nodiscard]] bool GetTriggerHitOnChildren() const { return TriggerHitOnChildren; }
	void SetTriggerHitOnChildren(bool triggerHitOnChildren) { TriggerHitOnChildren = triggerHitOnChildren; }
	[[nodiscard]] bool GetTriggerHitOnParentGameObject() const { return TriggerHitOnParentGameObject; }
	void SetTriggerHitOnParentGameObject(bool triggerHitOnParentGameObject) { TriggerHitOnParentGameObject = triggerHitOnParentGameObject; }
	[[nodiscard]] String GetLocatorPoolName() const { return LocatorPoolName; }
	void SetLocatorPoolName(const String &locatorPoolName) { LocatorPoolName = locatorPoolName; }
	[[nodiscard]] float GetRadius() const { return Radius; }
	void SetRadius(float radius) { Radius = radius; }
	[[nodiscard]] float GetTriggerEverySeconds() const { return TriggerEverySeconds; }
	void SetTriggerEverySeconds(float triggerEverySeconds) { TriggerEverySeconds = triggerEverySeconds; }
	[[nodiscard]] float GetProbabilityToApply() const { return ProbabilityToApply; }
	void SetProbabilityToApply(float probabilityToApply) { ProbabilityToApply = probabilityToApply; }
	[[nodiscard]] PackedStringArray GetModifierCategories() const { return ModifierCategories; }
	void SetModifierCategories(const PackedStringArray &modifierCategories) { ModifierCategories = modifierCategories; }
	[[nodiscard]] PackedStringArray GetDamageCategories() const { return DamageCategories; }
	void SetDamageCategories(const PackedStringArray &damageCategories) { DamageCategories = damageCategories; }
	[[nodiscard]] bool GetUseModifiedArea() const { return UseModifiedArea; }
	void SetUseModifiedArea(bool useModifiedArea) { UseModifiedArea = useModifiedArea; }
	[[nodiscard]] float GetRemainingTimeToNextTrigger() const { return _remainingTimeToNextTrigger; }
	void SetRemainingTimeToNextTrigger(float remainingTimeToNextTrigger) { _remainingTimeToNextTrigger = remainingTimeToNextTrigger; }
	[[nodiscard]] bool GetIsHarmless() const { return _is_harmless; }
	void SetIsHarmless(bool isHarmless) { _is_harmless = isHarmless; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedDamage() const { return _modifiedDamage; }
	void SetModifiedDamage(const Ref<ModifiedFloatValue> &modifiedDamage) { _modifiedDamage = modifiedDamage; }
	[[nodiscard]] Ref<Modifier> GetDamageModifier() const { return _damageModifier; }
	void SetDamageModifier(const Ref<Modifier> &damageModifier) { _damageModifier = damageModifier; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedAttackSpeed() const { return _modifiedAttackSpeed; }
	void SetModifiedAttackSpeed(const Ref<ModifiedFloatValue> &modifiedAttackSpeed) { _modifiedAttackSpeed = modifiedAttackSpeed; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedArea() const { return _modifiedArea; }
	void SetModifiedArea(const Ref<ModifiedFloatValue> &modifiedArea) { _modifiedArea = modifiedArea; }
	[[nodiscard]] int GetWeaponIndex() const { return _weapon_index; }
	void SetWeaponIndex(int weaponIndex) { _weapon_index = weaponIndex; }
	[[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
	void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
	[[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
	void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
};



#endif //AREAOFEFFECT_H
