
#ifndef GODOT_CLONE_LOCATORBASEDCOLLIDERHELPER_H
#define GODOT_CLONE_LOCATORBASEDCOLLIDERHELPER_H

#include "scene/2d/node_2d.h"
#include <core/object/ref_counted.h>

class LocatorSystem;
class GameObject;


class LocatorbasedColliderHelper  : public RefCounted {
	GDCLASS(LocatorbasedColliderHelper, RefCounted)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

	enum class ColliderTypes {
		Circle,
		Rectangle,
		CircleMotion
	};

	struct LocatorColliderData {
		LocatorbasedColliderHelper* BelongsTo = nullptr;
		Node2D* Owner = nullptr;
		bool IsActive = true;
		float ResetCollisionEverySeconds = 0;
		float RemainingTimeToReset = 0;
		TypedArray<String> LocatorPools;
		Array CurrentCollidingGameObjects;
		ColliderTypes ColliderType = ColliderTypes::Circle;
		float Radius = 0;
		float Width = 0;
		float Height = 0;
		float MotionDist = 0;
		Callable MotionDirectionCallable;
	};
	static LocalVector<LocatorColliderData> AllLocatorColliderHelpers;

	~LocatorbasedColliderHelper() override;

public:
	void InitializeAsCircle(Node2D* owner, TypedArray<String> locatorPools, float radius, float resetCollisionEverySeconds);
	void InitializeAsRectangle(Node2D* owner, TypedArray<String> locatorPools, float width, float height, float resetCollisionEverySeconds);
	void InitializeAsCircleMotion(Node2D* owner, TypedArray<String> locatorPools, float radius, float motionDist, float resetCollisionEverySeconds, Callable motionDirectionCallable);

	void SetHelperActive(bool active);
	TypedArray<GameObject> GetCurrentCollidingGameObjects();

	static void UpdateAllHelpers(LocatorSystem* locatorSystem, float delta);
};


#endif //GODOT_CLONE_LOCATORBASEDCOLLIDERHELPER_H
