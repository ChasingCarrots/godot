#include "FastRaybasedMover.h"

#include <core/variant/variant_utility.h>

#include "StatisticsValueData.h"

#include <scene/main/window.h>

LocalVector<FastRaybasedMover*> FastRaybasedMover::_allFastRaybasedMovers;

void FastRaybasedMover::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_Radius", "newRadius"), &FastRaybasedMover::SetRadius);
	ClassDB::bind_method(D_METHOD("get_Radius"), &FastRaybasedMover::GetRadius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Radius"), "set_Radius", "get_Radius");
	ClassDB::bind_method(D_METHOD("set_movement_speed", "newSpeed"), &FastRaybasedMover::SetMovementSpeed);
	ClassDB::bind_method(D_METHOD("get_MovementSpeed"), &FastRaybasedMover::GetMovementSpeed);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movementSpeed"), "set_movement_speed", "get_MovementSpeed");
	ClassDB::bind_method(D_METHOD("set_acceleration", "newAcceleration"), &FastRaybasedMover::SetAcceleration);
	ClassDB::bind_method(D_METHOD("get_Acceleration"), &FastRaybasedMover::GetAcceleration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "acceleration"), "set_acceleration", "get_Acceleration");
	ClassDB::bind_method(D_METHOD("set_Damping", "newDamping"), &FastRaybasedMover::SetDamping);
	ClassDB::bind_method(D_METHOD("get_Damping"), &FastRaybasedMover::GetDamping);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping"), "set_Damping", "get_Damping");
	ClassDB::bind_method(D_METHOD("set_StartSpeedRandomizer", "startSpeedRandomizer"), &FastRaybasedMover::SetStartSpeedRandomizer);
	ClassDB::bind_method(D_METHOD("get_StartSpeedRandomizer"), &FastRaybasedMover::GetStartSpeedRandomizer);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "startSpeedRandomizer"), "set_StartSpeedRandomizer", "get_StartSpeedRandomizer");
	ClassDB::bind_method(D_METHOD("set_DampingRandomizer", "dampingRandomizer"), &FastRaybasedMover::SetDampingRandomizer);
	ClassDB::bind_method(D_METHOD("get_DampingRandomizer"), &FastRaybasedMover::GetDampingRandomizer);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dampingRandomizer"), "set_DampingRandomizer", "get_DampingRandomizer");
	ClassDB::bind_method(D_METHOD("set_AllowMultipleHits", "allowMultipleHits"), &FastRaybasedMover::SetAllowMultipleHits);
	ClassDB::bind_method(D_METHOD("get_AllowMultipleHits"), &FastRaybasedMover::GetAllowMultipleHits);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allowMultipleHits"), "set_AllowMultipleHits", "get_AllowMultipleHits");
	ClassDB::bind_method(D_METHOD("set_AlignTransformWithMovement", "alignTransformWithMovement"), &FastRaybasedMover::SetAlignTransformWithMovement);
	ClassDB::bind_method(D_METHOD("get_AlignTransformWithMovement"), &FastRaybasedMover::GetAlignTransformWithMovement);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "alignTransformWithMovement"), "set_AlignTransformWithMovement", "get_AlignTransformWithMovement");
	ClassDB::bind_method(D_METHOD("set_HitLocatorPools", "hitLocatorPools"), &FastRaybasedMover::SetHitLocatorPools);
	ClassDB::bind_method(D_METHOD("get_HitLocatorPools"), &FastRaybasedMover::GetHitLocatorPools);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "HitLocatorPools", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_HitLocatorPools", "get_HitLocatorPools");
	ClassDB::bind_method(D_METHOD("set_RotationOverTime", "rotationOverTime"), &FastRaybasedMover::SetRotationOverTime);
	ClassDB::bind_method(D_METHOD("get_RotationOverTime"), &FastRaybasedMover::GetRotationOverTime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotationOverTime"), "set_RotationOverTime", "get_RotationOverTime");
	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "modifierCategories"), &FastRaybasedMover::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &FastRaybasedMover::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_ModifierCategories", "get_ModifierCategories");
	ClassDB::bind_method(D_METHOD("set_SpeedModifier", "speedModifier"), &FastRaybasedMover::SetSpeedModifier);
	ClassDB::bind_method(D_METHOD("get_SpeedModifier"), &FastRaybasedMover::GetSpeedModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "speedModifier"), "set_SpeedModifier", "get_SpeedModifier");
	ClassDB::bind_method(D_METHOD("set_AccelerationModifier", "accelerationModifier"), &FastRaybasedMover::SetAccelerationModifier);
	ClassDB::bind_method(D_METHOD("get_AccelerationModifier"), &FastRaybasedMover::GetAccelerationModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "accelerationModifier"), "set_AccelerationModifier", "get_AccelerationModifier");
	ClassDB::bind_method(D_METHOD("set_DampingModifier", "dampingModifier"), &FastRaybasedMover::SetDampingModifier);
	ClassDB::bind_method(D_METHOD("get_DampingModifier"), &FastRaybasedMover::GetDampingModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dampingModifier"), "set_DampingModifier", "get_DampingModifier");
	ClassDB::bind_method(D_METHOD("set_RotationModifier", "rotationModifier"), &FastRaybasedMover::SetRotationModifier);
	ClassDB::bind_method(D_METHOD("get_RotationModifier"), &FastRaybasedMover::GetRotationModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rotationModifier"), "set_RotationModifier", "get_RotationModifier");

	ClassDB::bind_method(D_METHOD("set_TotalMoved", "totalMoved"), &FastRaybasedMover::SetTotalMoved);
	ClassDB::bind_method(D_METHOD("get_TotalMoved"), &FastRaybasedMover::GetTotalMoved);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_totalMoved", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_TotalMoved", "get_TotalMoved");

	ADD_SIGNAL(MethodInfo("CollisionStarted", PropertyInfo(Variant::OBJECT, "otherNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("CollisionEnded", PropertyInfo(Variant::OBJECT, "otherNode", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("MovementStopped"));

	ClassDB::bind_method(D_METHOD("initialize_modifiers", "referenceParent"), &FastRaybasedMover::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &FastRaybasedMover::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &FastRaybasedMover::GetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats"), &FastRaybasedMover::get_display_stats);
	ClassDB::bind_method(D_METHOD("get_add_loca_msgids"), &FastRaybasedMover::get_add_loca_msgids);
	ClassDB::bind_method(D_METHOD("get_movement_speed"), &FastRaybasedMover::get_movement_speed);
	ClassDB::bind_method(D_METHOD("get_acceleration"), &FastRaybasedMover::get_acceleration);
	ClassDB::bind_method(D_METHOD("get_damping"), &FastRaybasedMover::get_damping);
	ClassDB::bind_method(D_METHOD("get_rotation_over_time"), &FastRaybasedMover::get_rotation_over_time);
	ClassDB::bind_method(D_METHOD("get_targetVelocity"), &FastRaybasedMover::get_targetVelocity);
	ClassDB::bind_method(D_METHOD("get_velocity"), &FastRaybasedMover::get_targetVelocity);
	ClassDB::bind_method(D_METHOD("set_velocity_offset", "velOffset"), &FastRaybasedMover::set_velocity_offset);
	ClassDB::bind_method(D_METHOD("set_velocity_offset_direct", "velOffset"), &FastRaybasedMover::set_velocity_offset_direct);
	ClassDB::bind_method(D_METHOD("get_velocity_offset"), &FastRaybasedMover::get_velocity_offset);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_velocity_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_velocity_offset_direct", "get_velocity_offset");
	ClassDB::bind_method(D_METHOD("set_targetDirection", "targetDir"), &FastRaybasedMover::set_targetDirection);
	ClassDB::bind_method(D_METHOD("get_targetDirection"), &FastRaybasedMover::get_targetDirection);
	ClassDB::bind_method(D_METHOD("get_worldPosition"), &FastRaybasedMover::get_worldPosition);
	ClassDB::bind_method(D_METHOD("set_worldPosition", "worldPos"), &FastRaybasedMover::set_worldPosition);
	ClassDB::bind_method(D_METHOD("add_velocity", "addVel"), &FastRaybasedMover::add_velocity);
	ClassDB::bind_method(D_METHOD("get_current_speed"), &FastRaybasedMover::get_current_speed);
	ClassDB::bind_method(D_METHOD("set_current_speed", "newSpeed"), &FastRaybasedMover::set_current_speed);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_currentSpeed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_current_speed", "get_current_speed");
	ClassDB::bind_method(D_METHOD("get_current_acceleration"), &FastRaybasedMover::get_current_acceleration);
	ClassDB::bind_method(D_METHOD("set_current_acceleration", "newAcceleration"), &FastRaybasedMover::set_current_acceleration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_currentAcceleration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_current_acceleration", "get_current_acceleration");
	ClassDB::bind_method(D_METHOD("calculateMotion"), &FastRaybasedMover::calculateMotion);
	ClassDB::bind_method(D_METHOD("copyAllTimeHitObjectsFromOther"), &FastRaybasedMover::copyAllTimeHitObjectsFromOther);
	ClassDB::bind_method(D_METHOD("get_randomized_speed"), &FastRaybasedMover::get_randomized_speed);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_randomizedSpeed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_randomized_speed");

	ClassDB::bind_static_method("FastRaybasedMover", D_METHOD("updateAllFastRaybasedMovers"), &FastRaybasedMover::updateAllFastRaybasedMovers);
}

void FastRaybasedMover::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
		case NOTIFICATION_ENTER_TREE:
			_enter_tree();
		break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
		break;
	}
}

void FastRaybasedMover::_exit_tree() {
	_allFastRaybasedMovers.erase(this);
}

void FastRaybasedMover::_enter_tree() {
	PROFILE_FUNCTION();
	_allFastRaybasedMovers.push_back(this);
}

void FastRaybasedMover::_ready() {
	PROFILE_FUNCTION();
	init_gameobject_component();
	initialize_modifiers(this);
	applyModifierCategories();

	float current_movementspeed = get_movement_speed();
	float current_acceleration = get_acceleration();
	current_acceleration *= (get_movement_speed() / movementSpeed);

	_randomizedDamping = get_damping() + VariantUtilityFunctions::randf() * dampingRandomizer * get_damping();
	_randomizedSpeed = current_movementspeed + VariantUtilityFunctions::randf() * startSpeedRandomizer * current_movementspeed;
	_randomizedAcceleration = (_randomizedSpeed / current_movementspeed) * current_acceleration;

	_currentSpeed = _randomizedSpeed;
	_currentAcceleration = _randomizedAcceleration;
}

TypedArray<StatisticsValueData> FastRaybasedMover::get_display_stats() {
	TypedArray<StatisticsValueData> collectedStats;
	if(_modifiedMovementSpeed.is_valid()) {
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedMovementSpeed, SpeedStatName, StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::SpeedDist, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}
	else if(!SpeedStatName.is_empty())
		collectedStats.append(
			StatisticsValueData::createNew(
				SpeedStatName, StatsCategory, "",
				movementSpeed, movementSpeed, {},
				StatisticsValueData::StatisticsFormatTypes::SpeedDist, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	return collectedStats;
}

TypedArray<String> FastRaybasedMover::get_add_loca_msgids() {
	TypedArray<String> retValue;
	retValue.append(SpeedStatName);
	return retValue;
}

Vector2 FastRaybasedMover::calculateMotion(float delta) {
	if(_currentAcceleration > 0)
		_currentSpeed += _currentAcceleration * delta;
	if(_currentAcceleration < 0) {
		bool speedBeforePositive = _currentSpeed > 0;
		_currentSpeed += _currentAcceleration * delta;
		if(speedBeforePositive && _currentSpeed <= 0)
			_emitStopped = true;
	}
	if(_currentSpeed > 0 && _randomizedDamping > 0) {
		_currentSpeed -= _currentSpeed * _randomizedDamping * delta;
		if(_currentSpeed < 1) {
			_currentSpeed = 0;
			_emitStopped = true;
		}
	}
	if(get_rotation_over_time() != 0)
		_currentDirection = _currentDirection.rotated(Math::deg_to_rad(get_rotation_over_time())*delta);

	if(_currentSpeed == 0)
		return {};

	return (_currentDirection + _velocity_offset) * _currentSpeed * delta;
}

static LocalVector<GameObject*> tempHits;
void FastRaybasedMover::updateAllFastRaybasedMovers(float delta) {
	PROFILE_FUNCTION();
	for(auto mover : _allFastRaybasedMovers) {
		if(!mover->_gameObject.is_valid())
			continue;

		Vector2 motion = mover->calculateMotion(delta);

		if(mover->_clearLastHitObjects) {
			mover->_lastHitObjects.clear();
			mover->_clearLastHitObjects = false;
		}
		mover->_totalMoved += motion.length();

		// bullet.gd uses the Node2Ds scale, so we just apply it to the radius like this:
		Vector2 moverPos = mover->get_global_position();
		Vector2 moverScale = mover->get_global_scale();
		float scaledRadius = mover->Radius * std::max(moverScale.x, moverScale.y);
		tempHits.clear();
		for(const String& pool : mover->HitLocatorPools) {
			LocatorSystem::FillWithGameObjectsInCircleMotion(pool, moverPos, scaledRadius, motion, tempHits);
		}
		for(GameObject* hitGameObj : tempHits) {
			if(!mover->allowMultipleHits && mover->_allTimeHitObjects.find(hitGameObj) != -1)
				continue;
			bool isInLastHitObjects = false;
			for(const auto lastHitObj : mover->_lastHitObjects) {
				if(lastHitObj.get_nocheck() == hitGameObj) {
					isInLastHitObjects = true;
					break;
				}
			}
			if(!isInLastHitObjects)
				mover->_lastHitObjects.push_back(SafeObjectPointer(hitGameObj));
		}
		mover->set_global_position(moverPos + motion);

		for(auto &node : mover->_lastHitObjects) {
			if(!node.is_valid() || node->is_queued_for_deletion())
				continue; // do not trigger signals for deleted nodes!
			if(mover->_currentHitObjects.find(node) == -1) {
				if(mover->_allTimeHitObjects.find(node.get_nocheck()) == -1)
					mover->_allTimeHitObjects.push_back(node.get_nocheck());
				mover->_currentHitObjects.push_back(node);
				mover->emit_signal("CollisionStarted", node.get_nocheck());
			}
			// else would be CollisionStay...
		}
		for(int i=mover->_currentHitObjects.size() - 1; i >= 0; --i) {
			GameObject* node = mover->_currentHitObjects[i].get();
			if(node == nullptr || node->is_queued_for_deletion()){
				mover->_currentHitObjects.remove_at(i);
				continue; // do not trigger signals for deleted nodes!
			}
			if(mover->_lastHitObjects.find(mover->_currentHitObjects[i]) == -1) {
				mover->emit_signal("CollisionEnded", node);
				mover->_currentHitObjects.remove_at(i);
			}
		}
		mover->_clearLastHitObjects = true;
		if(mover->_emitStopped) {
			mover->emit_signal("MovementStopped");
			mover->_emitStopped = false;
		}
	}
}

void FastRaybasedMover::initialize_modifiers(Node *referenceParent) {
	PROFILE_FUNCTION();
	if(!speedModifier.is_empty())
		_modifiedMovementSpeed = referenceParent->call("create_modified_float_value", movementSpeed, speedModifier);
	if(!accelerationModifier.is_empty())
		_modifiedAcceleration = referenceParent->call("create_modified_float_value", acceleration, accelerationModifier);
	if(!dampingModifier.is_empty())
		_modifiedDamping = referenceParent->call("create_modified_float_value", damping, dampingModifier);
	if(!rotationModifier.is_empty())
		_modifiedRotationOverTime = referenceParent->call("create_modified_float_value", rotationOverTime, rotationModifier);
}

void FastRaybasedMover::applyModifierCategories() {
	PROFILE_FUNCTION();
	Array modifierCatAsArr;
	for(const auto& modCat : ModifierCategories) modifierCatAsArr.append(modCat);

	if(_modifiedMovementSpeed.is_valid())
		_modifiedMovementSpeed->setModifierCategories(modifierCatAsArr);
	if(_modifiedAcceleration.is_valid())
		_modifiedAcceleration->setModifierCategories(modifierCatAsArr);
	if(_modifiedDamping.is_valid())
		_modifiedDamping->setModifierCategories(modifierCatAsArr);
	if(_modifiedRotationOverTime.is_valid())
		_modifiedRotationOverTime->setModifierCategories(modifierCatAsArr);
}

float FastRaybasedMover::get_movement_speed() {
	if(_modifiedMovementSpeed.is_valid())
		return _modifiedMovementSpeed->Value();
	return movementSpeed;
}

float FastRaybasedMover::get_acceleration() {
	if(_modifiedAcceleration.is_valid())
		return _modifiedAcceleration->Value();
	return acceleration;
}

float FastRaybasedMover::get_damping() {
	if(_modifiedDamping.is_valid())
		return _modifiedDamping->Value();
	return damping;
}

float FastRaybasedMover::get_rotation_over_time() {
	if(_modifiedRotationOverTime.is_valid())
		return _modifiedRotationOverTime->Value();
	return rotationOverTime;
}

Vector2 FastRaybasedMover::get_targetVelocity() {
	return (_currentDirection + _velocity_offset) * _currentSpeed;
}

void FastRaybasedMover::set_velocity_offset(Vector2 newVelOffset) {
	_velocity_offset = newVelOffset / _currentSpeed;
}

void FastRaybasedMover::set_targetDirection(Vector2 targetDirection) {
	_currentDirection = targetDirection;
	if(alignTransformWithMovement) {
		float rotationFromDirection = _currentDirection.angle();
		set_rotation(rotationFromDirection);
	}
}

Vector2 FastRaybasedMover::get_targetDirection() {
	return _currentDirection;
}
