
#ifndef GODOT_CLONE_LOCATORBASEDCOLLIDERHELPER_H
#define GODOT_CLONE_LOCATORBASEDCOLLIDERHELPER_H

#include "scene/2d/node_2d.h"
#include <core/object/ref_counted.h>
#include <experimental/vector>

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
		LocatorbasedColliderHelper* BelongsTo;
		Node2D* Owner;
		bool IsActive;
		float ResetCollisionEverySeconds;
		float RemainingTimeToReset;
		TypedArray<String> LocatorPools;
		Array CurrentCollidingGameObjects;
		ColliderTypes ColliderType;
		float Radius;
		float Width;
		float Height;
		float MotionDist;
		Callable MotionDirectionCallable;
	};
	static std::vector<LocatorColliderData> AllLocatorColliderHelpers;

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
