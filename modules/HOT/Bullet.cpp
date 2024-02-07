#include "Bullet.h"

#include "Health.h"

LocalVector<Bullet*> Bullet::_allBullets;

void Bullet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_EndLifeWhenStopped", "endLife"), &Bullet::SetEndLifeWhenStopped);
	ClassDB::bind_method(D_METHOD("get_EndLifeWhenStopped"), &Bullet::GetEndLifeWhenStopped);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "endLifeWhenStopped"), "set_EndLifeWhenStopped", "get_EndLifeWhenStopped");

	ClassDB::bind_method(D_METHOD("set_LifeTime", "newLifetime"), &Bullet::SetLifeTime);
	ClassDB::bind_method(D_METHOD("get_LifeTime"), &Bullet::GetLifeTime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifeTime"), "set_LifeTime", "get_LifeTime");

	ClassDB::bind_method(D_METHOD("set_DestroyWhenLifeEnded", "newDestroyWhenLifeEnded"), &Bullet::SetDestroyWhenLifeEnded);
	ClassDB::bind_method(D_METHOD("get_DestroyWhenLifeEnded"), &Bullet::GetDestroyWhenLifeEnded);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "destroyWhenLifeEnded"), "set_DestroyWhenLifeEnded", "get_DestroyWhenLifeEnded");

	ClassDB::bind_method(D_METHOD("set_EndLifeAfterNumberOfHits", "newEndLifeAfterNumberOfHits"), &Bullet::SetEndLifeAfterNumberOfHits);
	ClassDB::bind_method(D_METHOD("get_EndLifeAfterNumberOfHits"), &Bullet::GetEndLifeAfterNumberOfHits);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "endLifeAfterNumberOfHits"), "set_EndLifeAfterNumberOfHits", "get_EndLifeAfterNumberOfHits");

	ClassDB::bind_method(D_METHOD("set_EndLifeAfterDistance", "newEndLifeAfterDistance"), &Bullet::SetEndLifeAfterDistance);
	ClassDB::bind_method(D_METHOD("get_EndLifeAfterDistance"), &Bullet::GetEndLifeAfterDistance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "endLifeAfterDistance"), "set_EndLifeAfterDistance", "get_EndLifeAfterDistance");

	ClassDB::bind_method(D_METHOD("set_EndLifeAfterDistanceModifier", "newEndLifeAfterDistanceModifier"), &Bullet::SetEndLifeAfterDistanceModifier);
	ClassDB::bind_method(D_METHOD("get_EndLifeAfterDistanceModifier"), &Bullet::GetEndLifeAfterDistanceModifier);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "endLifeAfterDistanceModifier"), "set_EndLifeAfterDistanceModifier", "get_EndLifeAfterDistanceModifier");

	ClassDB::bind_method(D_METHOD("set_EndLifeWhenBlocked", "newEndLifeWhenBlocked"), &Bullet::SetEndLifeWhenBlocked);
	ClassDB::bind_method(D_METHOD("get_EndLifeWhenBlocked"), &Bullet::GetEndLifeWhenBlocked);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "endLifeWhenBlocked"), "set_EndLifeWhenBlocked", "get_EndLifeWhenBlocked");

	ClassDB::bind_method(D_METHOD("set_ModifierCategories", "newModifierCategories"), &Bullet::SetModifierCategories);
	ClassDB::bind_method(D_METHOD("get_ModifierCategories"), &Bullet::GetModifierCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ModifierCategories", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_ModifierCategories", "get_ModifierCategories");

	ClassDB::bind_method(D_METHOD("set_IsCharacterBaseNode", "newIsCharacterBaseNode"), &Bullet::SetIsCharacterBaseNode);
	ClassDB::bind_method(D_METHOD("get_IsCharacterBaseNode"), &Bullet::GetIsCharacterBaseNode);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "IsCharacterBaseNode"), "set_IsCharacterBaseNode", "get_IsCharacterBaseNode");

	ClassDB::bind_method(D_METHOD("set_DelayEndOfLifeByOneFrame", "newDelayEndOfLifeByOneFrame"), &Bullet::SetDelayEndOfLifeByOneFrame);
	ClassDB::bind_method(D_METHOD("get_DelayEndOfLifeByOneFrame"), &Bullet::GetDelayEndOfLifeByOneFrame);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "DelayEndOfLifeByOneFrame"), "set_DelayEndOfLifeByOneFrame", "get_DelayEndOfLifeByOneFrame");

	ClassDB::bind_method(D_METHOD("set_UseModifiedArea", "newUseModifiedArea"), &Bullet::SetUseModifiedArea);
	ClassDB::bind_method(D_METHOD("get_UseModifiedArea"), &Bullet::GetUseModifiedArea);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "UseModifiedArea"), "set_UseModifiedArea", "get_UseModifiedArea");

	ADD_GROUP("Curved Movement Parameters", "");
	ClassDB::bind_method(D_METHOD("set_MovementCurvatureAngle", "newMovementCurvatureAngle"), &Bullet::SetMovementCurvatureAngle);
	ClassDB::bind_method(D_METHOD("get_MovementCurvatureAngle"), &Bullet::GetMovementCurvatureAngle);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MovementCurvatureAngle"), "set_MovementCurvatureAngle", "get_MovementCurvatureAngle");

	ClassDB::bind_method(D_METHOD("set_MovementCurvatureDistance", "newMovementCurvatureDistance"), &Bullet::SetMovementCurvatureDistance);
	ClassDB::bind_method(D_METHOD("get_MovementCurvatureDistance"), &Bullet::GetMovementCurvatureDistance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "MovementCurvatureDistance"), "set_MovementCurvatureDistance", "get_MovementCurvatureDistance");

	ADD_GROUP("Quest", "");
	ClassDB::bind_method(D_METHOD("set_TrackDistanceTraveled", "newTrackDistanceTraveled"), &Bullet::SetTrackDistanceTraveled);
	ClassDB::bind_method(D_METHOD("get_TrackDistanceTraveled"), &Bullet::GetTrackDistanceTraveled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "TrackDistanceTraveled"), "set_TrackDistanceTraveled", "get_TrackDistanceTraveled");

	ClassDB::bind_method(D_METHOD("set_TrackForIndex", "newTrackForIndex"), &Bullet::SetTrackForIndex);
	ClassDB::bind_method(D_METHOD("get_TrackForIndex"), &Bullet::GetTrackForIndex);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "TrackForIndex"), "set_TrackForIndex", "get_TrackForIndex");


	ClassDB::bind_method(D_METHOD("set_RemainingLifeTime", "newRemainingLifeTime"), &Bullet::SetRemainingLifeTime);
	ClassDB::bind_method(D_METHOD("get_RemainingLifeTime"), &Bullet::GetRemainingLifeTime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_remainingLifeTime", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_RemainingLifeTime", "get_RemainingLifeTime");

	ClassDB::bind_method(D_METHOD("set_NumberOfHits", "newNumberOfHits"), &Bullet::SetNumberOfHits);
	ClassDB::bind_method(D_METHOD("get_NumberOfHits"), &Bullet::GetNumberOfHits);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_numberOfHits", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_NumberOfHits", "get_NumberOfHits");

	ClassDB::bind_method(D_METHOD("set_DistanceTraveled", "newDistanceTraveled"), &Bullet::SetDistanceTraveled);
	ClassDB::bind_method(D_METHOD("get_DistanceTraveled"), &Bullet::GetDistanceTraveled);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "_distanceTraveled", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_DistanceTraveled", "get_DistanceTraveled");

	ClassDB::bind_method(D_METHOD("set_ModifiedNumberOfHits", "newModifiedNumberOfHits"), &Bullet::SetModifiedNumberOfHits);
	ClassDB::bind_method(D_METHOD("get_ModifiedNumberOfHits"), &Bullet::GetModifiedNumberOfHits);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_modifiedNumberOfHits", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_ModifiedNumberOfHits", "get_ModifiedNumberOfHits");

	ClassDB::bind_method(D_METHOD("set_StatsCategory", "statsCategory"), &Bullet::SetStatsCategory);
	ClassDB::bind_method(D_METHOD("get_StatsCategory"), &Bullet::GetStatsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatsCategory"), "set_StatsCategory", "get_StatsCategory");

	ClassDB::bind_method(D_METHOD("set_StatsDisplayType", "statsDisplayType"), &Bullet::SetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_StatsDisplayType"), &Bullet::GetStatsDisplayType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "StatsDisplayType", PROPERTY_HINT_ENUM, "PlayerBaseStats,AbilityStats"), "set_StatsDisplayType", "get_StatsDisplayType");

	ADD_SIGNAL(MethodInfo("OnHit", PropertyInfo(Variant::OBJECT, "nodeHit", PROPERTY_HINT_RESOURCE_TYPE, "GameObject"),  PropertyInfo(Variant::INT, "hitNumber")));
	ADD_SIGNAL(MethodInfo("OnEndOfLife"));
	ADD_SIGNAL(MethodInfo("OnTimeReduced",  PropertyInfo(Variant::FLOAT,"remainingTimeFactor")));

	ClassDB::bind_static_method("Bullet", D_METHOD("updateAllBullets"), &Bullet::updateAllBullets);

	ClassDB::bind_method(D_METHOD("get_display_stats_type"), &Bullet::GetStatsDisplayType);
	ClassDB::bind_method(D_METHOD("get_display_stats"), &Bullet::get_display_stats);
	ClassDB::bind_method(D_METHOD("initialize_modifiers"), &Bullet::initialize_modifiers);
	ClassDB::bind_method(D_METHOD("applyModifierCategories"), &Bullet::applyModifierCategories);
	ClassDB::bind_method(D_METHOD("transformCategories"), &Bullet::transformCategories);
	ClassDB::bind_method(D_METHOD("sizeWasUpdated"), &Bullet::sizeWasUpdated);
	ClassDB::bind_method(D_METHOD("collisionWithNode"), &Bullet::collisionWithNode);
	ClassDB::bind_method(D_METHOD("endLife"), &Bullet::endLife);
	ClassDB::bind_method(D_METHOD("set_homing_target"), &Bullet::set_homing_target);
	ClassDB::bind_method(D_METHOD("get_maximum_distance"), &Bullet::get_maximum_distance);
	ClassDB::bind_method(D_METHOD("_on_hit_damage_applied"), &Bullet::_on_hit_damage_applied);
}

void Bullet::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_enter_tree();
		break;
		case NOTIFICATION_EXIT_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_exit_tree();
		break;
	}
}

void Bullet::_ready() {
	_distanceTraveled = 0;
	_numberOfHits = 0;
	init_gameobject_component();
	if(!_gameObject.is_valid()) return;
	if(endLifeWhenStopped)
		_gameObject->connectToSignal("MovementStopped", callable_mp(this, &Bullet::endLife));
	_gameObject->connectToSignal("CollisionStarted", callable_mp(this, &Bullet::collisionWithNode));
	_directionSetter = _gameObject->getChildNodeWithMethod("set_targetDirection");
	_speedProvider = _gameObject->getChildNodeWithMethod("get_movement_speed");

	initialize_modifiers(this);

	_modifiedSize->connect("ValueUpdated", callable_mp(this, &Bullet::sizeWasUpdated));
	_remainingLifeTime = _modifiedLifetime->Value();
	_lastLifeTimeFactorEmitted = 99;

	sizeWasUpdated(_modifiedSize->Value(), _modifiedSize->Value());
	if(endLifeWhenBlocked)
		_gameObject->connectToSignal("DamageApplied", callable_mp(this, &Bullet::_on_hit_damage_applied));
}

void Bullet::_enter_tree() {
	_allBullets.push_back(this);
}

void Bullet::_exit_tree() {
	_allBullets.erase(this);
	_modifiedNumberOfHits.unref();
	_modifiedSize.unref();
	_modifiedLifetime.unref();
	_modifiedMaxDistance.unref();
}

void Bullet::updateAllBullets(float delta) {
	PROFILE_FUNCTION();
	for(auto bullet : _allBullets) {
		if(bullet->_remainingLifeTime > 0) {
			bullet->_remainingLifeTime -= delta;
			const float lifeTimeFactor = std::max(0.0f, bullet->_remainingLifeTime / bullet->_modifiedLifetime->Value());
			if(lifeTimeFactor == 0 || bullet->_lastLifeTimeFactorEmitted - lifeTimeFactor > 0.05f) {
				bullet->emit_signal("OnTimeReduced", lifeTimeFactor);
				bullet->_lastLifeTimeFactorEmitted = lifeTimeFactor;
			}
			if(bullet->_remainingLifeTime <= 0) {
				bullet->endLife();
				continue;
			}
		}

		if(bullet->_directionSetter.is_valid() && bullet->MovementCurvatureAngle > 0.0 && bullet->MovementCurvatureDistance > 0.0) {
			Vector2 dir = bullet->_directionSetter->call("get_targetDirection");
			bullet->_directionSetter->call("set_targetDirection", dir.rotated(
				Math::deg_to_rad(bullet->MovementCurvatureAngle) / (bullet->MovementCurvatureDistance * delta)));
		}

		if(bullet->_directionSetter.is_valid() && bullet->_targetPositionProvider.is_valid())
			bullet->_directionSetter->call("set_targetDirection",
				(static_cast<Vector2>(bullet->_targetPositionProvider->call("get_worldPosition")) - bullet->get_gameobject_worldposition()).normalized());

		if(bullet->get_maximum_distance() > 0) {
			if(bullet->_speedProvider.is_valid() && bullet->_speedProvider->has_method("get_current_speed")) {
				bullet->_distanceTraveled += static_cast<float>(bullet->_speedProvider->call("get_current_speed")) * delta;
				if(bullet->_distanceTraveled >= bullet->get_maximum_distance())
					bullet->endLife();
			}
		}
	}
}

TypedArray<StatisticsValueData> Bullet::get_display_stats() {
	TypedArray<StatisticsValueData> collectedStats;
	if(_modifiedLifetime.is_valid()) {
		collectedStats.append(
		StatisticsValueData::createFromModifiedValue(
			_modifiedLifetime, tr("Max. Duration (Force)"), StatsCategory,
			StatisticsValueData::StatisticsFormatTypes::Time, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}
	if(_modifiedSize.is_valid()) {
		collectedStats.append(
		StatisticsValueData::createFromModifiedValue(
			_modifiedSize, tr("Size (Area)"), StatsCategory,
			StatisticsValueData::StatisticsFormatTypes::Percentage, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}
	if(_modifiedNumberOfHits.is_valid() && _modifiedNumberOfHits->Value() < 9999) {
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedNumberOfHits, tr("Piercing (Force)"), StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::FlatValue, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}
	if(_modifiedMaxDistance.is_valid() && _modifiedMaxDistance->Value() > 0) {
		collectedStats.append(
			StatisticsValueData::createFromModifiedValue(
				_modifiedMaxDistance, tr("Max. Distance"), StatsCategory,
				StatisticsValueData::StatisticsFormatTypes::Distance, StatisticsValueData::StatisticsSpecialInfoTypes::None));
	}

	return collectedStats;
}

void Bullet::initialize_modifiers(Node *referenceParent) {
	_modifiedNumberOfHits = referenceParent->call("createModifiedFloatValue", endLifeAfterNumberOfHits, "Force");
	_modifiedSize = referenceParent->call("createModifiedFloatValue", 1, "Area");
	_modifiedLifetime = referenceParent->call("createModifiedFloatValue", lifeTime, "Force");
	_modifiedMaxDistance = referenceParent->call("createModifiedFloatValue", endLifeAfterDistance, endLifeAfterDistanceModifier);
	applyModifierCategories();
}

void Bullet::applyModifierCategories() {
	Array modifierCatAsArr;
	for(const auto& modCat : ModifierCategories) modifierCatAsArr.append(modCat);

	_modifiedNumberOfHits->setModifierCategories(modifierCatAsArr);
	_modifiedMaxDistance->setModifierCategories(modifierCatAsArr);
	_modifiedSize->setModifierCategories(modifierCatAsArr);
	_modifiedLifetime->setModifierCategories(modifierCatAsArr);
}

void Bullet::transformCategories(String onlyWithModifierCategory, TypedArray<String> addModifierCategories, TypedArray<String> removeModifierCategories, TypedArray<String> addDamageCategories, TypedArray<String> removeDamageCategories) {
	if(!ModifierCategories.has(onlyWithModifierCategory))
		return;
	for(int i=0; i < addModifierCategories.size(); ++i)
		if (!ModifierCategories.has(addModifierCategories[i]))
			ModifierCategories.append(addModifierCategories[i]);
	for (int i=0; i < removeDamageCategories.size(); ++i)
		ModifierCategories.erase(removeDamageCategories[i]);
	applyModifierCategories();
}

void Bullet::sizeWasUpdated(float sizebefore, float newSize) {
	if(UseModifiedArea)
		_gameObject->set_scale(Vector2(1,1) * newSize);
}


void Bullet::collisionWithNode(Node *node) {
	PROFILE_FUNCTION();
	if(_numberOfHits < _modifiedNumberOfHits->Value()) {
		node = GameObject::getGameObjectInParents(node);
		if(node == nullptr || node->is_queued_for_deletion())
			// collision doesn't seem relevant!
			return;
		emit_signal("OnHit", node, _numberOfHits);
		_numberOfHits += 1;
		if(_numberOfHits >= _modifiedNumberOfHits->Value())
			endLife();
	}
}

void Bullet::endLife() {
	PROFILE_FUNCTION();
	emit_signal("OnEndOfLife");
	if(TrackDistanceTraveled && _speedProvider.is_valid() && static_cast<float>(_speedProvider->get("_totalMoved")) > 0.0f)
		GameObject::Global()->get("QuestPool").call(
			"notify_distance_moved", static_cast<float>(_speedProvider->get("_totalMoved")) / 18.0f, TrackForIndex);
	if(!destroyWhenLifeEnded)
		return;
	if(DelayEndOfLifeByOneFrame) {
		if(!get_tree()->is_connected("process_frame", callable_mp(this, &Bullet::endLifePart2)))
			get_tree()->connect("process_frame", callable_mp(this, &Bullet::endLifePart2), CONNECT_ONE_SHOT);
	}
	else
		endLifePart2();
}

void Bullet::endLifePart2() {
	Node* parent = get_parent();
	if(parent != nullptr)
		parent->queue_free();
}


void Bullet::set_homing_target(Node *targetPositionProvider) {
	_targetPositionProvider = targetPositionProvider;
}

float Bullet::get_maximum_distance() {
	if(endLifeAfterDistance <= 0)
		return 0;
	return _modifiedMaxDistance->Value();
}

void Bullet::_on_hit_damage_applied(const TypedArray<String> &damageCategories, float _damageAmount, const Array &applyReturn, GameObject *_targetNode, bool _isCritical) {
	if(static_cast<int>(applyReturn[0]) == static_cast<int>(ApplyDamageResult::Blocked)) {
		_numberOfHits = 9999999;
		_gameObject->queue_free();
	}
}
