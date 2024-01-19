#ifndef MONSTERINPUT_H
#define MONSTERINPUT_H

#include "GameObjectComponents.h"

class MonsterInput : public GameObjectComponent {
	GDCLASS(MonsterInput, GameObjectComponent)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _exit_tree();
	void _enter_tree();
	void _notification(int p_notification);

private:
	static LocalVector<MonsterInput*> _allMonsterInputs;

public:
	bool SetPlayerAsTargetOnSpawn = true;
	float StopWhenInRange = 0.0;
	bool KillSelfWhenInStopRange = false;
	int MovePattern = 0;
	float MovementCurvatureAngle = 30.0;
	float MovementCurvatureDistance = 300.0;
	float MinOffsetX = -64.0;
	float MaxOffsetX = 64.0;
	float MinOffsetY = -64.0;
	float MaxOffsetY = 64.0;
	Vector2 LaneDirection = Vector2(84, -64);
	float LaneDivergenceMaxRange = 200;
	float LaneDivergenceMinRange = 20;
	float MinMotionDuration = 1.0;
	float MaxMotionDuration = 1.0;
	float MinLoiteringDuration = 0.0;
	float MaxLoiteringDuration = 0.0;

	SafeObjectPointer<Node> targetDirectionSetter;
	SafeObjectPointer<Node> targetFacingSetter;
	Vector2 input_direction;
	float loitering_counter;
	SafeObjectPointer<Node> _targetPosProvider;
	Vector2 targetOffset;
	SafeObjectPointer<Node> _targetOverrideProvider;

	void _ready();
	static void updateAllMonsterInputs(float delta);
	void set_target(GameObject* targetNode);
	Vector2 get_inputWalkDir();
	Vector2 get_aimDirection();

	[[nodiscard]] bool GetSetPlayerAsTargetOnSpawn() const { return SetPlayerAsTargetOnSpawn; }
	void SetSetPlayerAsTargetOnSpawn(bool setPlayerAsTargetOnSpawn) { SetPlayerAsTargetOnSpawn = setPlayerAsTargetOnSpawn; }
	[[nodiscard]] float GetStopWhenInRange() const { return StopWhenInRange; }
	void SetStopWhenInRange(float stopWhenInRange) { StopWhenInRange = stopWhenInRange; }
	[[nodiscard]] bool GetKillSelfWhenInStopRange() const { return KillSelfWhenInStopRange; }
	void SetKillSelfWhenInStopRange(bool killSelfWhenInStopRange) { KillSelfWhenInStopRange = killSelfWhenInStopRange; }
	[[nodiscard]] int GetMovePattern() const { return MovePattern; }
	void SetMovePattern(int movePattern) { MovePattern = movePattern; }
	[[nodiscard]] float GetMovementCurvatureAngle() const { return MovementCurvatureAngle; }
	void SetMovementCurvatureAngle(float movementCurvatureAngle) { MovementCurvatureAngle = movementCurvatureAngle; }
	[[nodiscard]] float GetMovementCurvatureDistance() const { return MovementCurvatureDistance; }
	void SetMovementCurvatureDistance(float movementCurvatureDistance) { MovementCurvatureDistance = movementCurvatureDistance; }
	[[nodiscard]] float GetMinOffsetX() const { return MinOffsetX; }
	void SetMinOffsetX(float minOffsetX) { MinOffsetX = minOffsetX; }
	[[nodiscard]] float GetMaxOffsetX() const { return MaxOffsetX; }
	void SetMaxOffsetX(float maxOffsetX) { MaxOffsetX = maxOffsetX; }
	[[nodiscard]] float GetMinOffsetY() const { return MinOffsetY; }
	void SetMinOffsetY(float minOffsetY) { MinOffsetY = minOffsetY; }
	[[nodiscard]] float GetMaxOffsetY() const { return MaxOffsetY; }
	void SetMaxOffsetY(float maxOffsetY) { MaxOffsetY = maxOffsetY; }
	[[nodiscard]] Vector2 GetLaneDirection() const { return LaneDirection; }
	void SetLaneDirection(const Vector2 &laneDirection) { LaneDirection = laneDirection; }
	[[nodiscard]] float GetLaneDivergenceMaxRange() const { return LaneDivergenceMaxRange; }
	void SetLaneDivergenceMaxRange(float laneDivergenceMaxRange) { LaneDivergenceMaxRange = laneDivergenceMaxRange; }
	[[nodiscard]] float GetLaneDivergenceMinRange() const { return LaneDivergenceMinRange; }
	void SetLaneDivergenceMinRange(float laneDivergenceMinRange) { LaneDivergenceMinRange = laneDivergenceMinRange; }
	[[nodiscard]] float GetMinMotionDuration() const { return MinMotionDuration; }
	void SetMinMotionDuration(float minMotionDuration) { MinMotionDuration = minMotionDuration; }
	[[nodiscard]] float GetMaxMotionDuration() const { return MaxMotionDuration; }
	void SetMaxMotionDuration(float maxMotionDuration) { MaxMotionDuration = maxMotionDuration; }
	[[nodiscard]] float GetMinLoiteringDuration() const { return MinLoiteringDuration; }
	void SetMinLoiteringDuration(float minLoiteringDuration) { MinLoiteringDuration = minLoiteringDuration; }
	[[nodiscard]] float GetMaxLoiteringDuration() const { return MaxLoiteringDuration; }
	void SetMaxLoiteringDuration(float maxLoiteringDuration) { MaxLoiteringDuration = maxLoiteringDuration; }
	[[nodiscard]] Node* GetTargetDirectionSetter() { return targetDirectionSetter.get(); }
	void SetTargetDirectionSetter(Node* newTargetDirectionSetter) { targetDirectionSetter = newTargetDirectionSetter; }
	[[nodiscard]] Node* GetTargetFacingSetter() { return targetFacingSetter.get(); }
	void SetTargetFacingSetter(Node* newTargetFacingSetter) { targetFacingSetter = newTargetFacingSetter; }
	[[nodiscard]] Vector2 GetInputDirection() const { return input_direction; }
	void SetInputDirection(const Vector2 &inputDirection) { input_direction = inputDirection; }
	[[nodiscard]] float GetLoiteringCounter() const { return loitering_counter; }
	void SetLoiteringCounter(float loiteringCounter) { loitering_counter = loiteringCounter; }
	[[nodiscard]] Node* GetTargetPosProvider() { return _targetPosProvider.get(); }
	void SetTargetPosProvider(Node* targetPosProvider) { _targetPosProvider = targetPosProvider; }
	[[nodiscard]] Vector2 GetTargetOffset() const { return targetOffset; }
	void SetTargetOffset(const Vector2 &newTargetOffset) { targetOffset = newTargetOffset; }
	[[nodiscard]] Node* GetTargetOverrideProvider() { return _targetOverrideProvider.get(); }
	void SetTargetOverrideProvider(Node* targetOverrideProvider) { _targetOverrideProvider = targetOverrideProvider; }
};



#endif //MONSTERINPUT_H
