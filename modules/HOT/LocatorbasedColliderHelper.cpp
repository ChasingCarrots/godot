#include "LocatorbasedColliderHelper.h"
#include "GameObject.h"
#include "LocatorSystem.h"
#include "core/profiling.h"

std::vector<LocatorbasedColliderHelper::LocatorColliderData> LocatorbasedColliderHelper::AllLocatorColliderHelpers;

void LocatorbasedColliderHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("InitializeAsCircle", "OwnerNode", "LocatorPools", "Radius", "ResetCollisionEverySeconds"), &LocatorbasedColliderHelper::InitializeAsCircle);
	ClassDB::bind_method(D_METHOD("InitializeAsRectangle", "OwnerNode", "LocatorPools", "Width", "Height", "ResetCollisionEverySeconds"), &LocatorbasedColliderHelper::InitializeAsRectangle);
	ClassDB::bind_method(D_METHOD("InitializeAsCircleMotion", "OwnerNode", "LocatorPools", "Radius", "MotionDist", "ResetCollisionEverySeconds", "TargetDirectionCallable"), &LocatorbasedColliderHelper::InitializeAsCircleMotion);
	ClassDB::bind_method(D_METHOD("SetHelperActive", "Active"), &LocatorbasedColliderHelper::SetHelperActive);
	ClassDB::bind_method(D_METHOD("GetCurrentCollidingGameObjects"), &LocatorbasedColliderHelper::GetCurrentCollidingGameObjects);

	ClassDB::bind_static_method("LocatorbasedColliderHelper", D_METHOD("UpdateAllHelpers", "LocatorSystem", "delta"), &LocatorbasedColliderHelper::UpdateAllHelpers);

	ADD_SIGNAL(MethodInfo("CollisionStarted", PropertyInfo(Variant::OBJECT, "GamoObject")));
	ADD_SIGNAL(MethodInfo("CollisionEnded", PropertyInfo(Variant::OBJECT, "GamoObject")));
}

LocatorbasedColliderHelper::~LocatorbasedColliderHelper() {
	for (auto& data : AllLocatorColliderHelpers) {
		if(data.BelongsTo == this) {
			// we only set the BelongsTo to null, so that the main
			// loop can then erase the superfluous element from
			// the array without disturbing the iteration...
			data.BelongsTo = nullptr;
			break;
		}
	}
}

void LocatorbasedColliderHelper::InitializeAsCircle(Node2D *owner, TypedArray<String> locatorPools, float radius, float resetCollisionEverySeconds) {
	auto myData = LocatorColliderData();
	myData.BelongsTo = this;
	myData.ColliderType = ColliderTypes::Circle;
	myData.Owner = owner;
	myData.LocatorPools = locatorPools;
	myData.Radius = radius;
	myData.ResetCollisionEverySeconds = resetCollisionEverySeconds;

	if(resetCollisionEverySeconds <= 0) myData.RemainingTimeToReset = 999999;
	else myData.RemainingTimeToReset = resetCollisionEverySeconds;

	AllLocatorColliderHelpers.push_back(myData);
}

void LocatorbasedColliderHelper::InitializeAsRectangle(Node2D *owner, TypedArray<String> locatorPools, float width, float height, float resetCollisionEverySeconds) {
	auto myData = LocatorColliderData();
	myData.BelongsTo = this;
	myData.ColliderType = ColliderTypes::Rectangle;
	myData.Owner = owner;
	myData.LocatorPools = locatorPools;
	myData.Width = width;
	myData.Height = height;
	myData.ResetCollisionEverySeconds = resetCollisionEverySeconds;
	if(resetCollisionEverySeconds <= 0) myData.RemainingTimeToReset = 999999;
	else myData.RemainingTimeToReset = resetCollisionEverySeconds;

	AllLocatorColliderHelpers.push_back(myData);
}

void LocatorbasedColliderHelper::InitializeAsCircleMotion(Node2D *owner, TypedArray<String> locatorPools, float radius, float motionDist, float resetCollisionEverySeconds, Callable motionDirectionCallable) {
	auto myData = LocatorColliderData();
	myData.BelongsTo = this;
	myData.ColliderType = ColliderTypes::CircleMotion;
	myData.Owner = owner;
	myData.LocatorPools = locatorPools;
	myData.Radius = radius;
	myData.MotionDist = motionDist;
	myData.ResetCollisionEverySeconds = resetCollisionEverySeconds;
	if(resetCollisionEverySeconds <= 0) myData.RemainingTimeToReset = 999999;
	else myData.RemainingTimeToReset = resetCollisionEverySeconds;
	myData.MotionDirectionCallable = motionDirectionCallable;

	AllLocatorColliderHelpers.push_back(myData);
}

void LocatorbasedColliderHelper::UpdateAllHelpers(LocatorSystem* locatorSystem, float delta) {
	PROFILE_FUNCTION()
	Array temp_lastCollisions;
	auto dataIter = AllLocatorColliderHelpers.begin();
	while(dataIter != AllLocatorColliderHelpers.end()) {
		if(dataIter->BelongsTo != nullptr) {
			if(!dataIter->IsActive) {
				++dataIter;
				continue;
			}
			dataIter->RemainingTimeToReset -= delta;
			if(dataIter->RemainingTimeToReset <= 0) {
				dataIter->CurrentCollidingGameObjects.clear();
				dataIter->RemainingTimeToReset += dataIter->ResetCollisionEverySeconds;
			}
			// arrays are assigned by reference! so we need another temp
			// variable to essentially swap the last and current arrays
			Array temp_save = temp_lastCollisions;
			temp_lastCollisions = dataIter->CurrentCollidingGameObjects;
			dataIter->CurrentCollidingGameObjects = temp_save;
			dataIter->CurrentCollidingGameObjects.clear();
			Point2 pos = dataIter->Owner->get_global_position();
			float scaleFactor = dataIter->Owner->get_global_scale().x;
			switch(dataIter->ColliderType) {
				case ColliderTypes::Circle:
					for (int poolIndex = 0; poolIndex < dataIter->LocatorPools.size(); ++poolIndex) {
						dataIter->CurrentCollidingGameObjects.append_array(
								locatorSystem->GetGameObjectsInCircle(
									dataIter->LocatorPools[poolIndex],
									pos,
									dataIter->Radius)
						);
					}
					break;
				case ColliderTypes::Rectangle: {
					float minX = pos.x - dataIter->Width / 2.0f;
					float maxX = pos.x + dataIter->Width / 2.0f;
					float minY = pos.y - dataIter->Height / 2.0f;
					float maxY = pos.y + dataIter->Height / 2.0f;
					for (int poolIndex = 0; poolIndex < dataIter->LocatorPools.size(); ++poolIndex) {
						dataIter->CurrentCollidingGameObjects.append_array(
								locatorSystem->GetGameObjectsInRectangle(
										dataIter->LocatorPools[poolIndex],
										minX, maxX, minY, maxY));
					}
				} break;
				case ColliderTypes::CircleMotion:
					Vector2 motionVector = Vector2(1,0) * scaleFactor;
					if(dataIter->MotionDirectionCallable.is_valid())
						motionVector = dataIter->MotionDirectionCallable.call();
					motionVector *= dataIter->MotionDist;
					for (int poolIndex = 0; poolIndex < dataIter->LocatorPools.size(); ++poolIndex) {
						dataIter->CurrentCollidingGameObjects.append_array(
								locatorSystem->GetGameObjectsInCircleMotion(
										dataIter->LocatorPools[poolIndex],
										pos,
										dataIter->Radius * scaleFactor,
										motionVector)
						);
					}
					break;
			}
			for (int lastIndex = 0; lastIndex < temp_lastCollisions.size(); ++lastIndex) {
				if(!dataIter->CurrentCollidingGameObjects.has(temp_lastCollisions[lastIndex])) {
					// it is possible that our current BelongsTo node was invalidated in one of
					// the Collision callbacks! so we have to check for that every loop...
					if(dataIter->BelongsTo == nullptr)
						break;
					Object* validEndCollisionObj = temp_lastCollisions[lastIndex].get_validated_object();
					if(validEndCollisionObj == nullptr)
						continue;
					dataIter->BelongsTo->emit_signal("CollisionEnded", temp_lastCollisions[lastIndex]);
				}
			}
			for (int currentIndex = 0; currentIndex < dataIter->CurrentCollidingGameObjects.size(); ++currentIndex) {
				if(!temp_lastCollisions.has(dataIter->CurrentCollidingGameObjects[currentIndex])) {
					// it is possible that our current BelongsTo node was invalidated in one of
					// the Collision callbacks! so we have to check for that every loop...
					if(dataIter->BelongsTo == nullptr)
						break;
					dataIter->BelongsTo->emit_signal("CollisionStarted", dataIter->CurrentCollidingGameObjects[currentIndex]);
				}
			}

			++dataIter;
		}
		else {
			dataIter = AllLocatorColliderHelpers.erase(dataIter);
		}
	}
}
TypedArray<GameObject> LocatorbasedColliderHelper::GetCurrentCollidingGameObjects() {
	for (auto& data : AllLocatorColliderHelpers) {
		if (data.BelongsTo == this) {
			return data.CurrentCollidingGameObjects;
		}
	}
	return {};
}

void LocatorbasedColliderHelper::SetHelperActive(bool active) {
	for (auto& data : AllLocatorColliderHelpers) {
		if (data.BelongsTo == this) {
			data.IsActive = active;
		}
	}
}
