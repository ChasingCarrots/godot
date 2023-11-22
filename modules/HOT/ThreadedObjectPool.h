#ifndef GODOT_CLONE_THREADEDOBJECTPOOL_H
#define GODOT_CLONE_THREADEDOBJECTPOOL_H

#include "core/os/semaphore.h"
#include "scene/resources/packed_scene.h"
#include <core/object/ref_counted.h>
#include <core/os/thread.h>
#include <core/templates/local_vector.h>

class ThreadedObjectPool  : public RefCounted {
	GDCLASS(ThreadedObjectPool, RefCounted)

public:
	enum MaxInstancesReachedBehaviour {
		ReturnNull,
		RecycleOldest
	};

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

	Ref<PackedScene> _sceneToInstantiate;
	uint32_t _maxNumberOfInstances = 10;
	MaxInstancesReachedBehaviour _maxBehaviour = MaxInstancesReachedBehaviour::ReturnNull;

	LocalVector<ObjectID> _availableObjects;
	LocalVector<ObjectID> _inUseObjects;

	struct InstanceCreationData {
		Node* CreatedInstance = nullptr;
		Callable CreationCallback;
	};
	LocalVector<InstanceCreationData> _instanceCreationQueue;
	Mutex _instanceListMutex;
	Semaphore _instanceCreationSemaphore;
	bool _endThread = false;
	Thread _instanceCreationThread;
	static void instance_creation_thread_loop(void *p_ud);

	~ThreadedObjectPool();

public:

	void init_with_scene(String scenePath, int maxNumberOfInstances, MaxInstancesReachedBehaviour maxBehaviour);

	void get_instance(Callable instanceCreatedCallback);
	void return_instance(Node* instance);

	void run_callbacks();

	void clear_all_instances();
};

VARIANT_ENUM_CAST(ThreadedObjectPool::MaxInstancesReachedBehaviour);

#endif //GODOT_CLONE_THREADEDOBJECTPOOL_H
