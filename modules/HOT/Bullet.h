#ifndef BULLET_H
#define BULLET_H

#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class Bullet : public GameObjectComponent {
	GDCLASS(Bullet, GameObjectComponent)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

	static LocalVector<Bullet*> _allBullets;

	bool endLifeWhenStopped = true;
	float lifeTime = 1;
	bool destroyWhenLifeEnded = false;
	float endLifeAfterNumberOfHits = 1;
	float endLifeAfterDistance = 0;
	String endLifeAfterDistanceModifier = "Range";
	bool endLifeWhenBlocked = true;
	PackedStringArray ModifierCategories = {"Projectile"};
	bool IsCharacterBaseNode = false;
	bool DelayEndOfLifeByOneFrame = false;
	bool UseModifiedArea = true;
	float MovementCurvatureAngle = 0.0;
	float MovementCurvatureDistance = 0.0;
	bool TrackDistanceTraveled = false;
	int TrackForIndex = 0;

	float _remainingLifeTime = 0;
	float _lastLifeTimeFactorEmitted = 0;
	int _numberOfHits = 0;
	float _distanceTraveled = 0;

	Ref<ModifiedFloatValue> _modifiedNumberOfHits;
	Ref<ModifiedFloatValue> _modifiedSize;
	Ref<ModifiedFloatValue> _modifiedLifetime;
	Ref<ModifiedFloatValue> _modifiedMaxDistance;

	SafeObjectPointer<Node> _directionSetter;
	SafeObjectPointer<Node> _speedProvider;
	SafeObjectPointer<Node> _targetPositionProvider;


	StatisticsValueData::StatisticsTypes StatsDisplayType = StatisticsValueData::PlayerBaseStats;
	String StatsCategory = "Defensive Values";

	void _ready();
	void _enter_tree();
	void _exit_tree();

	void endLifePart2();
public:
	static void updateAllBullets(float delta);

	TypedArray<StatisticsValueData> get_display_stats();
	void initialize_modifiers(Node* referenceParent);
	void applyModifierCategories();
	void transformCategories(String onlyWithModifierCategory, TypedArray<String> addModifierCategories, TypedArray<String> removeModifierCategories, TypedArray<String> addDamageCategories, TypedArray<String> removeDamageCategories);
	void sizeWasUpdated(float sizebefore, float newSize);
	void collisionWithNode(Node* node);
	void endLife();
	void set_homing_target(Node* targetPositionProvider);
	float get_maximum_distance();
	void _on_hit_damage_applied(const TypedArray<String> &damageCategories, float _damageAmount, const Array &applyReturn, GameObject *_targetNode, bool _isCritical);

	[[nodiscard]] bool GetEndLifeWhenStopped() const { return endLifeWhenStopped; }
	void SetEndLifeWhenStopped(bool new_endLifeWhenStopped) { this->endLifeWhenStopped = new_endLifeWhenStopped; }
	[[nodiscard]] float GetLifeTime() const { return lifeTime; }
	void SetLifeTime(float new_lifeTime) { this->lifeTime = new_lifeTime; }
	[[nodiscard]] bool GetDestroyWhenLifeEnded() const { return destroyWhenLifeEnded; }
	void SetDestroyWhenLifeEnded(bool new_destroyWhenLifeEnded) { this->destroyWhenLifeEnded = new_destroyWhenLifeEnded; }
	[[nodiscard]] float GetEndLifeAfterNumberOfHits() const { return endLifeAfterNumberOfHits; }
	void SetEndLifeAfterNumberOfHits(float new_endLifeAfterNumberOfHits) { this->endLifeAfterNumberOfHits = new_endLifeAfterNumberOfHits; }
	[[nodiscard]] float GetEndLifeAfterDistance() const { return endLifeAfterDistance; }
	void SetEndLifeAfterDistance(float new_endLifeAfterDistance) { this->endLifeAfterDistance = new_endLifeAfterDistance; }
	[[nodiscard]] String GetEndLifeAfterDistanceModifier() const { return endLifeAfterDistanceModifier; }
	void SetEndLifeAfterDistanceModifier(const String &new_endLifeAfterDistanceModifier) { this->endLifeAfterDistanceModifier = new_endLifeAfterDistanceModifier; }
	[[nodiscard]] bool GetEndLifeWhenBlocked() const { return endLifeWhenBlocked; }
	void SetEndLifeWhenBlocked(bool new_endLifeWhenBlocked) { this->endLifeWhenBlocked = new_endLifeWhenBlocked; }
	[[nodiscard]] PackedStringArray GetModifierCategories() const { return ModifierCategories; }
	void SetModifierCategories(const PackedStringArray &modifierCategories) { ModifierCategories = modifierCategories; }
	[[nodiscard]] bool GetIsCharacterBaseNode() const { return IsCharacterBaseNode; }
	void SetIsCharacterBaseNode(bool new_isCharacterBaseNode) { IsCharacterBaseNode = new_isCharacterBaseNode; }
	[[nodiscard]] bool GetDelayEndOfLifeByOneFrame() const { return DelayEndOfLifeByOneFrame; }
	void SetDelayEndOfLifeByOneFrame(bool delayEndOfLifeByOneFrame) { DelayEndOfLifeByOneFrame = delayEndOfLifeByOneFrame; }
	[[nodiscard]] bool GetUseModifiedArea() const { return UseModifiedArea; }
	void SetUseModifiedArea(bool new_useModifiedArea) { UseModifiedArea = new_useModifiedArea; }
	[[nodiscard]] float GetMovementCurvatureAngle() const { return MovementCurvatureAngle; }
	void SetMovementCurvatureAngle(float new_movementCurvatureAngle) { MovementCurvatureAngle = new_movementCurvatureAngle; }
	[[nodiscard]] float GetMovementCurvatureDistance() const { return MovementCurvatureDistance; }
	void SetMovementCurvatureDistance(float new_movementCurvatureDistance) { MovementCurvatureDistance = new_movementCurvatureDistance; }
	[[nodiscard]] bool GetTrackDistanceTraveled() const { return TrackDistanceTraveled; }
	void SetTrackDistanceTraveled(bool new_trackDistanceTraveled) { TrackDistanceTraveled = new_trackDistanceTraveled; }
	[[nodiscard]] int GetTrackForIndex() const { return TrackForIndex; }
	void SetTrackForIndex(int new_trackForIndex) { TrackForIndex = new_trackForIndex; }
	[[nodiscard]] float GetRemainingLifeTime() const { return _remainingLifeTime; }
	void SetRemainingLifeTime(float new_remainingLifeTime) { _remainingLifeTime = new_remainingLifeTime; }
	[[nodiscard]] int GetNumberOfHits() const { return _numberOfHits; }
	void SetNumberOfHits(int new_numberOfHits) { _numberOfHits = new_numberOfHits; }
	[[nodiscard]] float GetDistanceTraveled() const { return _distanceTraveled; }
	void SetDistanceTraveled(float new_distanceTraveled) { _distanceTraveled = new_distanceTraveled; }
	[[nodiscard]] Ref<ModifiedFloatValue> GetModifiedNumberOfHits() const { return _modifiedNumberOfHits; }
	void SetModifiedNumberOfHits(ModifiedFloatValue* modifiedNumberOfHits) { _modifiedNumberOfHits.reference_ptr(modifiedNumberOfHits); }
	[[nodiscard]] StatisticsValueData::StatisticsTypes GetStatsDisplayType() const { return StatsDisplayType; }
	void SetStatsDisplayType(StatisticsValueData::StatisticsTypes new_statsDisplayType) { StatsDisplayType = new_statsDisplayType; }
	[[nodiscard]] String GetStatsCategory() const { return StatsCategory; }
	void SetStatsCategory(const String &statsCategory) { StatsCategory = statsCategory; }
};



#endif //BULLET_H
