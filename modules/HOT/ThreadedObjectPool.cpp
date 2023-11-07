#include "ThreadedObjectPool.h"
#include "core/profiling.h"

void ThreadedObjectPool::_bind_methods() {
	BIND_ENUM_CONSTANT(ReturnNull)
	BIND_ENUM_CONSTANT(RecycleOldest)

	ClassDB::bind_method(D_METHOD("init_with_scene", "scenePath", "maxNumberOfInstances", "maxBehaviour"), &ThreadedObjectPool::init_with_scene);
	ClassDB::bind_method(D_METHOD("get_instance", "instanceCreatedCallback"), &ThreadedObjectPool::get_instance);
	ClassDB::bind_method(D_METHOD("return_instance", "instance"), &ThreadedObjectPool::return_instance);
	ClassDB::bind_method(D_METHOD("run_callbacks"), &ThreadedObjectPool::run_callbacks);
	ClassDB::bind_method(D_METHOD("clear_all_instances"), &ThreadedObjectPool::clear_all_instances);
}

void ThreadedObjectPool::instance_creation_thread_loop(void *p_ud) {
	// the scene instantiation calls some methods that are deemed unsafe, so we have to
	// disable the thread safety for us...
	set_current_thread_safe_for_nodes(true);

	ThreadedObjectPool* pool = static_cast<ThreadedObjectPool *>(p_ud);
	while(!pool->_endThread) {
		pool->_instanceCreationSemaphore.wait();
		if(pool->_endThread)
			break;

		while(true) {
			InstanceCreationData workOnCreation;

			{
				MutexLock lock(pool->_instanceListMutex);
				for (uint32_t i = 0; i < pool->_instanceCreationQueue.size(); ++i) {
					if (pool->_instanceCreationQueue[i].CreatedInstance == nullptr) {
						// we'll remove this one from the queue, so that
						// we can work on it without holding the lock!
						workOnCreation = pool->_instanceCreationQueue[i];
						pool->_instanceCreationQueue.remove_at_unordered(i);
						break;
					}
				}
			}

			if(workOnCreation.CreationCallback.is_valid()) {
				{
					PROFILE_FUNCTION("InstantiatePooledObject")
					workOnCreation.CreatedInstance = pool->_sceneToInstantiate->instantiate();
					if (workOnCreation.CreatedInstance->has_method("managed_by_pool")) {
						workOnCreation.CreatedInstance->call("managed_by_pool", pool);
					}
				}
				// our work is done! let's put it back on the queue, so the main thread
				// can trigger the callback and remove it from the queue for good.
				{
					MutexLock lock(pool->_instanceListMutex);
					pool->_instanceCreationQueue.push_back(workOnCreation);
				}
			}
			else {
				// no more creation jobs in the queue! let's break this loop!
				break;
			}
		}
	}
}

ThreadedObjectPool::~ThreadedObjectPool() {
	clear_all_instances();
	if(_instanceCreationThread.is_started()) {
		_endThread = true;
		_instanceCreationSemaphore.post();
		_instanceCreationThread.wait_to_finish();
	}
}

void ThreadedObjectPool::init_with_scene(String scenePath, int maxNumberOfInstances, ThreadedObjectPool::MaxInstancesReachedBehaviour maxBehaviour) {
	_maxNumberOfInstances = (uint32_t)maxNumberOfInstances;
	_maxBehaviour = maxBehaviour;
	_sceneToInstantiate = ResourceLoader::load(scenePath);
	if(!_sceneToInstantiate.is_valid())
		return;
	_instanceCreationThread.start(instance_creation_thread_loop, this);
}

void ThreadedObjectPool::get_instance(Callable instanceCreatedCallback) {
	PROFILE_FUNCTION()
	if(_availableObjects.is_empty() && _inUseObjects.size() >= _maxNumberOfInstances) {
		switch(_maxBehaviour) {
			case ReturnNull:
				instanceCreatedCallback.call(Variant());
				return;
			case RecycleOldest:
				ObjectID oldestID = _inUseObjects[0];
				// we have to use the less performant remove_at here, because this vector
				// is actually ordered: oldest first, newest last!
				_inUseObjects.remove_at(0);
				Object* oldestObj = ObjectDB::get_instance(oldestID);
				if(oldestObj == nullptr) {
					print_error("ThreadedObjectPool had an invalid Object in its pool. Don't queue_free the pooled Objects, but return them with 'your_object_pool.return_instance(your_object)'!");
					// no fallback here!
					return;
				}
				// we'll push it back again, so that it is inUse, but at the 'newest' place
				_inUseObjects.push_back(oldestID);
				if(oldestObj->has_method("recycle_pooled_object"))
					oldestObj->call("recycle_pooled_object");
				instanceCreatedCallback.call(oldestObj);
				return;
		}
	}

	while(!_availableObjects.is_empty()) {
		ObjectID returnObjID = _availableObjects[_availableObjects.size()-1];
		_availableObjects.remove_at_unordered(_availableObjects.size()-1);
		Object* returnObj = ObjectDB::get_instance(returnObjID);
		if(returnObj == nullptr) {
			print_error("ThreadedObjectPool had an invalid Object in its pool. Don't queue_free the pooled Objects, but return them with 'your_object_pool.return_instance(your_object)'!");
			continue;
		}
		if(returnObj->has_method("recycle_pooled_object"))
			returnObj->call("recycle_pooled_object");
		_inUseObjects.push_back(returnObjID);
		instanceCreatedCallback.call(returnObj);
		return;
	}
	InstanceCreationData creationData;
	creationData.CreationCallback = instanceCreatedCallback;
	MutexLock lock(_instanceListMutex);
	_instanceCreationQueue.push_back(creationData);
	_instanceCreationSemaphore.post();
}

void ThreadedObjectPool::return_instance(Node *instance) {
	if(instance->get_parent() != nullptr) {
		instance->get_parent()->remove_child(instance);
	}
	_inUseObjects.erase(instance->get_instance_id());
	_availableObjects.push_back(instance->get_instance_id());
}

void ThreadedObjectPool::run_callbacks() {
	PROFILE_FUNCTION()
	MutexLock lock(_instanceListMutex);
	uint32_t i = 0;
	while(i < _instanceCreationQueue.size()) {
		if(_instanceCreationQueue[i].CreatedInstance != nullptr) {
			if(_instanceCreationQueue[i].CreatedInstance->has_method("recycle_pooled_object"))
				_instanceCreationQueue[i].CreatedInstance->call("recycle_pooled_object");
			_inUseObjects.push_back(_instanceCreationQueue[i].CreatedInstance->get_instance_id());
			_instanceCreationQueue[i].CreationCallback.call(_instanceCreationQueue[i].CreatedInstance);
			_instanceCreationQueue.remove_at_unordered(i);
		}
		else {
			++i;
		}
	}
}
void ThreadedObjectPool::clear_all_instances() {
	MutexLock lock(_instanceListMutex);
	for(auto& creationData : _instanceCreationQueue) {
		if(creationData.CreatedInstance != nullptr)
			creationData.CreatedInstance->queue_free();
	}
	_instanceCreationQueue.clear();
	for(ObjectID objId : _availableObjects) {
		Node* obj = Object::cast_to<Node>(ObjectDB::get_instance(objId));
		if(obj == nullptr)
			continue;
		obj->queue_free();
	}
	_availableObjects.clear();
	for(ObjectID objId : _inUseObjects) {
		Node* obj = Object::cast_to<Node>(ObjectDB::get_instance(objId));
		if(obj == nullptr)
			continue;
		obj->queue_free();
	}
	_inUseObjects.clear();
}
