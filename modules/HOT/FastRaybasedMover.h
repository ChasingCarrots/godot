#ifndef FASTRAYBASEDMOVER_H
#define FASTRAYBASEDMOVER_H

#include "GameObjectComponents.h"
#include "LocatorSystem.h"
#include "StatisticsValueData.h"

class FastRaybasedMover : public GameObjectComponent2D {
	GDCLASS(FastRaybasedMover, GameObjectComponent2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _exit_tree();
	void _enter_tree();
	void _ready();
	void _notification(int p_notification);

private:
	static LocalVector<FastRaybasedMover*> _allFastRaybasedMovers;
	Ref<LocatorSystem> _locatorSystem;
	float _randomizedDamping = 0;
	float _randomizedSpeed = 0;
	float _randomizedAcceleration = 0;

	Ref<ModifiedFloatValue> _modifiedMovementSpeed;
	Ref<ModifiedFloatValue> _modifiedAcceleration;
	Ref<ModifiedFloatValue> _modifiedDamping;
	Ref<ModifiedFloatValue> _modifiedRotationOverTime;

	Vector2 _currentDirection;
	float _currentSpeed = 0;
	float _currentAcceleration = 0;
	LocalVector<GameObject *> _allTimeHitObjects;
	LocalVector<SafeObjectPointer<GameObject>> _currentHitObjects;
	LocalVector<SafeObjectPointer<GameObject>> _lastHitObjects;
	bool _clearLastHitObjects = false;
	bool _emitStopped = false;
	Vector2 _velocity_offset;

	StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
	String StatsCategory;
	String SpeedStatName;
	TypedArray<StatisticsValueData> get_display_stats();

	TypedArray<String> get_add_loca_msgids();

public:
	float Radius = 10;
	float movementSpeed = 100;
	float acceleration = 0;
	float damping = 0;
	float startSpeedRandomizer = 0;
	float dampingRandomizer = 0;
	bool allowMultipleHits = false;
	bool alignTransformWithMovement = true;
	PackedStringArray HitLocatorPools = { "Enemies", "Breakables" };
	float rotationOverTime = 0;
	PackedStringArray ModifierCategories = { "Projectile" };
	String speedModifier;
	String accelerationModifier;
	String dampingModifier;
	String rotationModifier;
	float _totalMoved = 0.0;

	static void updateAllFastRaybasedMovers(float delta);

	void initialize_modifiers(Node* referenceParent);
	void applyModifierCategories();
	Vector2 calculateMotion(float delta);
	float get_movement_speed();
	float get_acceleration();
	void set_acceleration(float newAccel) { acceleration = newAccel; }
	float get_damping();
	float get_rotation_over_time();
	Vector2 get_targetVelocity();
	Vector2 get_velocity_offset() { return _velocity_offset; }
	void set_velocity_offset_direct(Vector2 newVelOffset) { _velocity_offset = newVelOffset; }
	void set_velocity_offset(Vector2 newVelOffset);
	void set_targetDirection(Vector2 targetDirection);
	Vector2 get_targetDirection();
	// no external deflection, but we have to have this function defined!
	void add_velocity(Vector2 addVel) {}
	float get_current_speed() { return _currentSpeed; }
	void set_current_speed(float newSpeed) { _currentSpeed = newSpeed; }
	Vector2 get_worldPosition() { return get_global_position(); }
	void set_worldPosition(Vector2 newWorldPos) { set_global_position(newWorldPos); }
	float get_current_acceleration() { return _currentAcceleration; }
	void set_current_acceleration(float newAccel) { _currentAcceleration = newAccel; }
	float get_randomized_speed() { return _randomizedSpeed; }

	void copyAllTimeHitObjectsFromOther(FastRaybasedMover* fastRaybasedMover) {
		_allTimeHitObjects = fastRaybasedMover->_allTimeHitObjects;
	}

	[[nodiscard]] float GetRadius() const { return Radius; }
	void SetRadius(float radius) { Radius = radius; }
	[[nodiscard]] float GetMovementSpeed() const { return movementSpeed; }
	void SetMovementSpeed(float new_movementSpeed) { this->movementSpeed = new_movementSpeed; }
	[[nodiscard]] float GetAcceleration() const { return acceleration; }
	void SetAcceleration(float new_acceleration) { this->acceleration = new_acceleration; }
	[[nodiscard]] float GetDamping() const { return damping; }
	void SetDamping(float new_damping) { this->damping = new_damping; }
	[[nodiscard]] float GetStartSpeedRandomizer() const { return startSpeedRandomizer; }
	void SetStartSpeedRandomizer(float new_startSpeedRandomizer) { this->startSpeedRandomizer = new_startSpeedRandomizer; }
	[[nodiscard]] float GetDampingRandomizer() const { return dampingRandomizer; }
	void SetDampingRandomizer(float new_dampingRandomizer) { this->dampingRandomizer = new_dampingRandomizer; }
	[[nodiscard]] bool GetAllowMultipleHits() const { return allowMultipleHits; }
	void SetAllowMultipleHits(bool new_allowMultipleHits) { this->allowMultipleHits = new_allowMultipleHits; }
	[[nodiscard]] bool GetAlignTransformWithMovement() const { return alignTransformWithMovement; }
	void SetAlignTransformWithMovement(bool new_alignTransformWithMovement) { this->alignTransformWithMovement = new_alignTransformWithMovement; }
	[[nodiscard]] PackedStringArray GetHitLocatorPools() const { return HitLocatorPools; }
	void SetHitLocatorPools(const PackedStringArray &hitLocatorPools) { HitLocatorPools = hitLocatorPools; }
	[[nodiscard]] float GetRotationOverTime() const { return rotationOverTime; }
	void SetRotationOverTime(float new_rotationOverTime) { this->rotationOverTime = new_rotationOverTime; }
	[[nodiscard]] PackedStringArray GetModifierCategories() const { return ModifierCategories; }
	void SetModifierCategories(const PackedStringArray &modifierCategories) { ModifierCategories = modifierCategories; }
	[[nodiscard]] String GetSpeedModifier() const { return speedModifier; }
	void SetSpeedModifier(const String &new_speedModifier) { this->speedModifier = new_speedModifier; }
	[[nodiscard]] String GetAccelerationModifier() const { return accelerationModifier; }
	void SetAccelerationModifier(const String &new_accelerationModifier) { this->accelerationModifier = new_accelerationModifier; }
	[[nodiscard]] String GetDampingModifier() const { return dampingModifier; }
	void SetDampingModifier(const String &new_dampingModifier) { this->dampingModifier = new_dampingModifier; }
	[[nodiscard]] String GetRotationModifier() const { return rotationModifier; }
	void SetRotationModifier(const String &new_rotationModifier) { this->rotationModifier = new_rotationModifier; }
	[[nodiscard]] float GetTotalMoved() const { return _totalMoved; }
	void SetTotalMoved(float totalMoved) { _totalMoved = totalMoved; }
	[[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
	void SetStatsDisplayType(StatisticsValueData::StatisticsTypes statsDisplayType) { StatsDisplayType = statsDisplayType; }
	[[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
	void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
};



#endif //FASTRAYBASEDMOVER_H
