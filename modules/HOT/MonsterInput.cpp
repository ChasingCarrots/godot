#include "MonsterInput.h"

#include <core/variant/variant_utility.h>
#include <scene/main/window.h>

LocalVector<MonsterInput*> MonsterInput::_allMonsterInputs;

void MonsterInput::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_SetPlayerAsTargetOnSpawn", "setPlayerAsTargetOnSpawn"), &MonsterInput::SetSetPlayerAsTargetOnSpawn);
	ClassDB::bind_method(D_METHOD("get_SetPlayerAsTargetOnSpawn"), &MonsterInput::GetSetPlayerAsTargetOnSpawn);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "SetPlayerAsTargetOnSpawn"), "set_SetPlayerAsTargetOnSpawn", "get_SetPlayerAsTargetOnSpawn");
	ClassDB::bind_method(D_METHOD("set_StopWhenInRange", "stopWhenInRange"), &MonsterInput::SetStopWhenInRange);
	ClassDB::bind_method(D_METHOD("get_StopWhenInRange"), &MonsterInput::GetStopWhenInRange);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "StopWhenInRange"), "set_StopWhenInRange", "get_StopWhenInRange");
	ClassDB::bind_method(D_METHOD("set_KillSelfWhenInStopRange", "killSelfWhenInStopRange"), &MonsterInput::SetKillSelfWhenInStopRange);
	ClassDB::bind_method(D_METHOD("get_KillSelfWhenInStopRange"), &MonsterInput::GetKillSelfWhenInStopRange);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "KillSelfWhenInStopRange"), "set_KillSelfWhenInStopRange", "get_KillSelfWhenInStopRange");
	ClassDB::bind_method(D_METHOD("set_MovePattern", "movePattern"), &MonsterInput::SetMovePattern);
	ClassDB::bind_method(D_METHOD("get_MovePattern"), &MonsterInput::GetMovePattern);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "MovePattern", PROPERTY_HINT_ENUM, "Linear,Curve,Offset,Lane"), "set_MovePattern", "get_MovePattern");
	ClassDB::bind_method(D_METHOD("set_MovementCurvatureAngle", "movementCurvatureAngle"), &MonsterInput::SetMovementCurvatureAngle);
	ClassDB::bind_method(D_METHOD("get_MovementCurvatureAngle"), &MonsterInput::GetMovementCurvatureAngle);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MovementCurvatureAngle"), "set_MovementCurvatureAngle", "get_MovementCurvatureAngle");
	ClassDB::bind_method(D_METHOD("set_MovementCurvatureDistance", "movementCurvatureDistance"), &MonsterInput::SetMovementCurvatureDistance);
	ClassDB::bind_method(D_METHOD("get_MovementCurvatureDistance"), &MonsterInput::GetMovementCurvatureDistance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MovementCurvatureDistance"), "set_MovementCurvatureDistance", "get_MovementCurvatureDistance");
	ClassDB::bind_method(D_METHOD("set_MinOffsetX", "minOffsetX"), &MonsterInput::SetMinOffsetX);
	ClassDB::bind_method(D_METHOD("get_MinOffsetX"), &MonsterInput::GetMinOffsetX);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MinOffsetX"), "set_MinOffsetX", "get_MinOffsetX");
	ClassDB::bind_method(D_METHOD("set_MaxOffsetX", "maxOffsetX"), &MonsterInput::SetMaxOffsetX);
	ClassDB::bind_method(D_METHOD("get_MaxOffsetX"), &MonsterInput::GetMaxOffsetX);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MaxOffsetX"), "set_MaxOffsetX", "get_MaxOffsetX");
	ClassDB::bind_method(D_METHOD("set_MinOffsetY", "minOffsetY"), &MonsterInput::SetMinOffsetY);
	ClassDB::bind_method(D_METHOD("get_MinOffsetY"), &MonsterInput::GetMinOffsetY);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MinOffsetY"), "set_MinOffsetY", "get_MinOffsetY");
	ClassDB::bind_method(D_METHOD("set_MaxOffsetY", "maxOffsetY"), &MonsterInput::SetMaxOffsetY);
	ClassDB::bind_method(D_METHOD("get_MaxOffsetY"), &MonsterInput::GetMaxOffsetY);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MaxOffsetY"), "set_MaxOffsetY", "get_MaxOffsetY");
	ClassDB::bind_method(D_METHOD("set_LaneDirection", "laneDirection"), &MonsterInput::SetLaneDirection);
	ClassDB::bind_method(D_METHOD("get_LaneDirection"), &MonsterInput::GetLaneDirection);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "LaneDirection"), "set_LaneDirection", "get_LaneDirection");
	ClassDB::bind_method(D_METHOD("set_LaneDivergenceMaxRange", "laneDivergenceMaxRange"), &MonsterInput::SetLaneDivergenceMaxRange);
	ClassDB::bind_method(D_METHOD("get_LaneDivergenceMaxRange"), &MonsterInput::GetLaneDivergenceMaxRange);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "LaneDivergenceMaxRange"), "set_LaneDivergenceMaxRange", "get_LaneDivergenceMaxRange");
	ClassDB::bind_method(D_METHOD("set_LaneDivergenceMinRange", "laneDivergenceMinRange"), &MonsterInput::SetLaneDivergenceMinRange);
	ClassDB::bind_method(D_METHOD("get_LaneDivergenceMinRange"), &MonsterInput::GetLaneDivergenceMinRange);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "LaneDivergenceMinRange"), "set_LaneDivergenceMinRange", "get_LaneDivergenceMinRange");
	ClassDB::bind_method(D_METHOD("set_MinMotionDuration", "minMotionDuration"), &MonsterInput::SetMinMotionDuration);
	ClassDB::bind_method(D_METHOD("get_MinMotionDuration"), &MonsterInput::GetMinMotionDuration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MinMotionDuration"), "set_MinMotionDuration", "get_MinMotionDuration");
	ClassDB::bind_method(D_METHOD("set_MaxMotionDuration", "maxMotionDuration"), &MonsterInput::SetMaxMotionDuration);
	ClassDB::bind_method(D_METHOD("get_MaxMotionDuration"), &MonsterInput::GetMaxMotionDuration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MaxMotionDuration"), "set_MaxMotionDuration", "get_MaxMotionDuration");
	ClassDB::bind_method(D_METHOD("set_MinLoiteringDuration", "minLoiteringDuration"), &MonsterInput::SetMinLoiteringDuration);
	ClassDB::bind_method(D_METHOD("get_MinLoiteringDuration"), &MonsterInput::GetMinLoiteringDuration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MinLoiteringDuration"), "set_MinLoiteringDuration", "get_MinLoiteringDuration");
	ClassDB::bind_method(D_METHOD("set_MaxLoiteringDuration", "maxLoiteringDuration"), &MonsterInput::SetMaxLoiteringDuration);
	ClassDB::bind_method(D_METHOD("get_MaxLoiteringDuration"), &MonsterInput::GetMaxLoiteringDuration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MaxLoiteringDuration"), "set_MaxLoiteringDuration", "get_MaxLoiteringDuration");

	ClassDB::bind_method(D_METHOD("set_targetDirectionSetter", "targetDirectionSetter"), &MonsterInput::SetTargetDirectionSetter);
	ClassDB::bind_method(D_METHOD("get_targetDirectionSetter"), &MonsterInput::GetTargetDirectionSetter);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "targetDirectionSetter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_targetDirectionSetter", "get_targetDirectionSetter");
	ClassDB::bind_method(D_METHOD("set_TargetFacingSetter", "targetFacingSetter"), &MonsterInput::SetTargetFacingSetter);
	ClassDB::bind_method(D_METHOD("get_TargetFacingSetter"), &MonsterInput::GetTargetFacingSetter);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "targetFacingSetter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_TargetFacingSetter", "get_TargetFacingSetter");
	ClassDB::bind_method(D_METHOD("set_Input_Direction", "input_direction"), &MonsterInput::SetInputDirection);
	ClassDB::bind_method(D_METHOD("get_Input_Direction"), &MonsterInput::GetInputDirection);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "input_Direction", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_Input_Direction", "get_Input_Direction");
	ClassDB::bind_method(D_METHOD("set_Loitering_Counter", "loitering_counter"), &MonsterInput::SetLoiteringCounter);
	ClassDB::bind_method(D_METHOD("get_Loitering_Counter"), &MonsterInput::GetLoiteringCounter);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loitering_counter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_Loitering_Counter", "get_Loitering_Counter");
	ClassDB::bind_method(D_METHOD("set_target", "targetNode"), &MonsterInput::SetTargetPosProvider);
	ClassDB::bind_method(D_METHOD("get_TargetPosProvider"), &MonsterInput::GetTargetPosProvider);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_targetPosProvider", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_target", "get_TargetPosProvider");
	ClassDB::bind_method(D_METHOD("set_TargetOffset", "targetOffset"), &MonsterInput::SetTargetOffset);
	ClassDB::bind_method(D_METHOD("get_TargetOffset"), &MonsterInput::GetTargetOffset);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "targetOffset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_TargetOffset", "get_TargetOffset");
	ClassDB::bind_method(D_METHOD("set_TargetOverrideProvider", "_targetOverrideProvider"), &MonsterInput::SetTargetOverrideProvider);
	ClassDB::bind_method(D_METHOD("get_TargetOverrideProvider"), &MonsterInput::GetTargetOverrideProvider);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_targetOverrideProvider", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_TargetOverrideProvider", "get_TargetOverrideProvider");

	ClassDB::bind_method(D_METHOD("get_inputWalkDir"), &MonsterInput::GetInputDirection);
	ClassDB::bind_method(D_METHOD("get_aimDirection"), &MonsterInput::GetInputDirection);

	ClassDB::bind_static_method("MonsterInput", D_METHOD("updateAllMonsterInputs"), &MonsterInput::updateAllMonsterInputs);

	ADD_SIGNAL(MethodInfo("input_dir_changed", PropertyInfo(Variant::VECTOR2, "dir_vector")));
	ADD_SIGNAL(MethodInfo("OnEndOfLife"));
}

void MonsterInput::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
		break;
		case NOTIFICATION_ENTER_TREE:
			_enter_tree();
		break;
	}
}

void MonsterInput::_exit_tree() {
	_allMonsterInputs.erase(this);
}

void MonsterInput::_enter_tree() {
	_allMonsterInputs.push_back(this);
}

void MonsterInput::_ready() {
	PROFILE_FUNCTION();
	init_gameobject_component();
	targetDirectionSetter = _gameObject->getChildNodeWithMethod("set_targetDirection");
	targetFacingSetter = _gameObject->getChildNodeWithMethod("set_facingDirection");
	_targetOverrideProvider = _gameObject->getChildNodeWithMethod("get_override_target_position");
	if (GameObject::World() == nullptr) {
		return;
	}
	GameObject* player = cast_to<GameObject>(GameObject::World()->get("Player"));

	if (SetPlayerAsTargetOnSpawn && player != nullptr && !player->is_queued_for_deletion())
		set_target(player);
	if(MovePattern == 2)
		targetOffset = Vector2(
			VariantUtilityFunctions::randf_range(MinOffsetX, MaxOffsetY),
			VariantUtilityFunctions::randf_range(MinOffsetY, MinOffsetY));
	else
		targetOffset = Vector2();
	set_process(false);
}

static OAHashMap<Node*, Vector2> NodePosCache;
Vector2 getPosOfPosProvider(SafeObjectPointer<Node>& posProvider) {
	Vector2 pos;
	Node* node = posProvider.get_nocheck();
	if(NodePosCache.lookup(node, pos))
		return pos;
	if(!posProvider.is_valid())
		return {};
	pos = node->call("get_worldPosition");
	NodePosCache.insert(node, pos);
	return pos;
}

void MonsterInput::updateAllMonsterInputs(float delta) {
	PROFILE_FUNCTION();
	NodePosCache.clear();
	for(auto monster : _allMonsterInputs) {
		Vector2 newInputDir;
		if(monster->_targetPosProvider.is_valid() && !monster->_targetPosProvider->is_queued_for_deletion()) {
			bool hasOverridePos = monster->_targetOverrideProvider.is_valid() && monster->_targetOverrideProvider->call("has_override_target_position");
			Vector2 targetPos;
			if(!hasOverridePos)
				targetPos = getPosOfPosProvider(monster->_targetPosProvider);
			else
				targetPos = monster->_targetOverrideProvider->call("get_override_target_position");
			Vector2 monsterPos = monster->get_gameobject_worldposition();

			if(hasOverridePos || monster->MovePattern == 0)
				// linear movement
					newInputDir = targetPos - monsterPos;
			else if(monster->MovePattern == 1) {
				// curved movement
				Vector2 target_vector = targetPos - monsterPos;
				float curve_factor = std::min(1.0f, std::max(0.0f,
					Math::inverse_lerp(8.0f, monster->MovementCurvatureDistance, target_vector.length())));
				newInputDir = target_vector.rotated(Math::deg_to_rad(Math::lerp(0.0f, monster->MovementCurvatureAngle, curve_factor)));
			}
			else if(monster->MovePattern == 2)
				// offset movement
					newInputDir = targetPos - monsterPos + monster->targetOffset;
			else if(monster->MovePattern == 3) {
				// lane movement
				Vector2 target_vector = targetPos - monsterPos;
				Vector2 laneInputDir = monster->LaneDirection * VariantUtilityFunctions::signf(target_vector.dot(monster->LaneDirection));
				if(monster->LaneDivergenceMaxRange > 0.0) {
					float directionWeight = std::min(1.0f, std::max(0.0f,
						Math::inverse_lerp(monster->LaneDivergenceMinRange,	monster->LaneDivergenceMaxRange, target_vector.length())));
					newInputDir = target_vector.slerp(laneInputDir, directionWeight);
				}
				else
					newInputDir = laneInputDir;
			}

			if((newInputDir - monster->targetOffset).length() <= monster->StopWhenInRange) {
				if(monster->KillSelfWhenInStopRange) {
					Array params;
					params.append(monster->_gameObject.get());
					monster->_gameObject->injectEmitSignal("Killed", params);
					// we also destroy the gameobject!
					monster->_gameObject->queue_free();
					return;
				}
				else {
					monster->targetFacingSetter->call("set_facingDirection", newInputDir);
					newInputDir = Vector2();
				}
			}
			else
				newInputDir = newInputDir.normalized();

			if(monster->MaxLoiteringDuration > 0.0f) {
				if(monster->loitering_counter == 0.0f)
					monster->loitering_counter = VariantUtilityFunctions::randf_range(monster->MinLoiteringDuration, monster->MaxLoiteringDuration);
				else if(monster->loitering_counter > 0.0f) {
					monster->loitering_counter = std::max(0.0f, std::min(9999.0f, monster->loitering_counter - delta));
					monster->targetFacingSetter->call("set_facingDirection", newInputDir);
					newInputDir = Vector2();
					if(monster->loitering_counter == 0.0f)
						monster->loitering_counter = -VariantUtilityFunctions::randf_range(monster->MinMotionDuration, monster->MaxMotionDuration);
				}
				else if(monster->loitering_counter < 0.0)
					monster->loitering_counter = std::max(-9999.0f, std::min(0.0f, monster->loitering_counter + delta));
			}
		}
		if(newInputDir != monster->input_direction) {
			monster->input_direction = newInputDir;
			monster->emit_signal("input_dir_changed", monster->input_direction);
			monster->targetDirectionSetter->call("set_targetDirection", monster->input_direction);
		}
	}
}

void MonsterInput::set_target(GameObject *targetNode) {
	_targetPosProvider = targetNode->getChildNodeWithMethod("get_worldPosition");
}

Vector2 MonsterInput::get_inputWalkDir() {
	return input_direction;
}

Vector2 MonsterInput::get_aimDirection() {
	// monsters walk to their target and aim to their target...
	return input_direction;
}
