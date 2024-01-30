#ifndef AREAOFEFFECT_H
#define AREAOFEFFECT_H

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

#include <scene/resources/packed_scene.h>

class AreaOfEffect  : public GameObjectComponent2D {
	GDCLASS(AreaOfEffect, GameObjectComponent2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _exit_tree();
	void _enter_tree();
	void _ready();
	void _notification(int p_notification);

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

	[[nodiscard]] int get_apply_damage() const {
		return ApplyDamage;
	}

	void set_apply_damage(const int apply_damage) {
		ApplyDamage = apply_damage;
	}

	[[nodiscard]] String get_rank_damage_modifier_method() const {
		return RankDamageModifierMethod;
	}

	void set_rank_damage_modifier_method(String rank_damage_modifier_method) {
		RankDamageModifierMethod = std::move(rank_damage_modifier_method);
	}

	[[nodiscard]] Ref<PackedScene> get_apply_node() const {
		return ApplyNode;
	}

	void set_apply_node(Ref<PackedScene> apply_node) {
		ApplyNode = apply_node;
	}

	[[nodiscard]] bool is_trigger_hit_on_children() const {
		return TriggerHitOnChildren;
	}

	void set_trigger_hit_on_children(const bool trigger_hit_on_children) {
		TriggerHitOnChildren = trigger_hit_on_children;
	}

	[[nodiscard]] bool is_trigger_hit_on_parent_game_object() const {
		return TriggerHitOnParentGameObject;
	}

	void set_trigger_hit_on_parent_game_object(const bool trigger_hit_on_parent_game_object) {
		TriggerHitOnParentGameObject = trigger_hit_on_parent_game_object;
	}

	[[nodiscard]] String get_locator_pool_name() const {
		return LocatorPoolName;
	}

	void set_locator_pool_name(String locator_pool_name) {
		LocatorPoolName = std::move(locator_pool_name);
	}

	[[nodiscard]] float get_radius() const {
		return Radius;
	}

	void set_radius(const float radius) {
		Radius = radius;
	}

	[[nodiscard]] float get_trigger_every_seconds() const {
		return TriggerEverySeconds;
	}

	void set_trigger_every_seconds(const float trigger_every_seconds) {
		TriggerEverySeconds = trigger_every_seconds;
	}

	[[nodiscard]] float get_probability_to_apply() const {
		return ProbabilityToApply;
	}

	void set_probability_to_apply(const float probability_to_apply) {
		ProbabilityToApply = probability_to_apply;
	}

	[[nodiscard]] PackedStringArray get_modifier_categories() const {
		return ModifierCategories;
	}

	void set_modifier_categories(PackedStringArray modifier_categories) {
		ModifierCategories = std::move(modifier_categories);
	}

	[[nodiscard]] PackedStringArray get_damage_categories() const {
		return DamageCategories;
	}

	void set_damage_categories(PackedStringArray damage_categories) {
		DamageCategories = std::move(damage_categories);
	}

	[[nodiscard]] bool is_use_modified_area() const {
		return UseModifiedArea;
	}

	void set_use_modified_area(const bool use_modified_area) {
		UseModifiedArea = use_modified_area;
	}

	[[nodiscard]] Ref<ModifiedFloatValue> get_modified_attack_speed() const {
		return _modifiedAttackSpeed;
	}

	void set_modified_attack_speed(Ref<ModifiedFloatValue> modified_attack_speed) {
		_modifiedAttackSpeed = std::move(modified_attack_speed);
	}

	[[nodiscard]] int get_weapon_index() const {
		return _weapon_index;
	}

	void set_weapon_index(const int weapon_index) {
		_weapon_index = weapon_index;
	}

	[[nodiscard]] StatisticsValueData::StatisticsTypes get_stats_display_type() const {
		return StatsDisplayType;
	}

	void set_stats_display_type(const StatisticsValueData::StatisticsTypes stats_display_type) {
		StatsDisplayType = stats_display_type;
	}

	[[nodiscard]] String get_stats_category() const {
		return StatsCategory;
	}

	void set_stats_category(String stats_category) {
		StatsCategory = std::move(stats_category);
	}

	[[nodiscard]] bool is_display_stats() const {
		return DisplayStats;
	}

	void set_display_stats(const bool display_stats) {
		DisplayStats = display_stats;
	}
};



#endif //AREAOFEFFECT_H
