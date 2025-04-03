#ifndef WORLD_H
#define WORLD_H

#include "Entity.h"
#include "System.h"

#include <scene/main/node.h>

namespace eves {

class World : public Node {
	GDCLASS(World, Node);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	Entity *CreateEntity();
	Entity *GetEntityByID(EntityID entityId);
	void DestroyEntity(Entity *entity);
	void DestroyEntityByID(EntityID entityId);

	void AddSystem(System* system);

	void Update();

private:
	LocalVector<Ref<System>> _systems;
};

} //namespace eves

#endif //WORLD_H
