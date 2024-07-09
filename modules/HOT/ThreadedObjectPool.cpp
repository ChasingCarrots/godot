#include "ThreadedObjectPool.h"
#include "core/profiling.h"

void ThreadedObjectPool::_bind_methods() {
	BIND_ENUM_CONSTANT(ReturnNull)
	BIND_ENUM_CONSTANT(RecycleOldest)
	BIND_ENUM_CONSTANT(GrowPool)

	ClassDB::bind_method(D_METHOD("init_with_scene", "scenePath", "maxNumberOfInstances", "maxBehaviour"), &ThreadedObjectPool::init_with_scene);
	ClassDB::bind_method(D_METHOD("get_instance", "instanceCreatedCallback"), &ThreadedObjectPool::get_instance);
	ClassDB::bind_method(D_METHOD("get_instance_unthreaded"), &ThreadedObjectPool::get_instance_unthreaded);
	ClassDB::bind_method(D_METHOD("return_instance", "instance"), &ThreadedObjectPool::return_instance);
	ClassDB::bind_method(D_METHOD("run_callbacks"), &ThreadedObjectPool::run_callbacks);
	ClassDB::bind_method(D_METHOD("clear_all_instances"), &ThreadedObjectPool::clear_all_instances);
	ClassDB::bind_method(D_METHOD("set_max_number_of_instances", "max_num_instances"), &ThreadedObjectPool::set_max_number_of_instances);
}

ThreadedObjectPool::~ThreadedObjectPool() {
	clear_all_instances();
}

void ThreadedObjectPool::init_with_scene_res(Ref<PackedScene> sceneRes, int maxNumberOfInstances, ThreadedObjectPool::MaxInstancesReachedBehaviour maxBehaviour) {
	_maxNumberOfInstances = (uint32_t)maxNumberOfInstances;
	_maxBehaviour = maxBehaviour;
	_sceneToInstantiate = sceneRes;
	if(!_sceneToInstantiate.is_valid())
		return;
}

void ThreadedObjectPool::init_with_scene(String scenePath, int maxNumberOfInstances, ThreadedObjectPool::MaxInstancesReachedBehaviour maxBehaviour) {
	_maxNumberOfInstances = (uint32_t)maxNumberOfInstances;
	_maxBehaviour = maxBehaviour;
	_sceneToInstantiate = ResourceLoader::load(scenePath);
	if(!_sceneToInstantiate.is_valid())
		return;
}

void ThreadedObjectPool::get_instance(Callable instanceCreatedCallback) {
	PROFILE_FUNCTION()
	ERR_FAIL_COND_MSG(!_sceneToInstantiate.is_valid(),"_instanceCreationQue empty in WorkerThread");

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
	{
		MutexLock lock(_instanceListMutex);
		_instanceCreationQueue.push_back(creationData);

		_taskIDsInQueue.push_back(
			WorkerThreadPool::get_singleton()->add_task(
			callable_mp(this, &ThreadedObjectPool::handleCreationQueueElement), false, "instantiate in ThreadPool"));
	}
}

void ThreadedObjectPool::handleCreationQueueElement() {
	if(_is_clearing_instances) {
		return;
	}
	InstanceCreationData workOnCreation;
	{
		MutexLock lock(_instanceListMutex);
		ERR_FAIL_COND_MSG(_instanceCreationQueue.is_empty(),"_instanceCreationQue empty in WorkerThread");
		workOnCreation = _instanceCreationQueue[0];
		_instanceCreationQueue.remove_at(0);
	}

	if(workOnCreation.CreationCallback.is_valid()) {
		{
			workOnCreation.CreatedInstance = _sceneToInstantiate->instantiate();
			if (workOnCreation.CreatedInstance->has_method("managed_by_pool")) {
				workOnCreation.CreatedInstance->call("managed_by_pool", this);
			}
		}
		// our work is done! let's put it back on the queue, so the main thread
		// can trigger the callback and remove it from the queue for good.
		{
			MutexLock lock(_instanceListMutex);
			_finishedCreations.push_back(workOnCreation);
		}
	}
}

Node* ThreadedObjectPool::get_instance_unthreaded() {
	PROFILE_FUNCTION();
	if(_availableObjects.is_empty() && _inUseObjects.size() >= _maxNumberOfInstances) {
		switch(_maxBehaviour) {
			case ReturnNull:
				return nullptr;
			case RecycleOldest:
				ObjectID oldestID = _inUseObjects[0];
			// we have to use the less performant remove_at here, because this vector
			// is actually ordered: oldest first, newest last!
			_inUseObjects.remove_at(0);
			Object* oldestObj = ObjectDB::get_instance(oldestID);
			if(oldestObj == nullptr) {
				print_error("ThreadedObjectPool had an invalid Object in its pool. Don't queue_free the pooled Objects, but return them with 'your_object_pool.return_instance(your_object)'!");
				// no fallback here!
				return nullptr;
			}
			// we'll push it back again, so that it is inUse, but at the 'newest' place
			_inUseObjects.push_back(oldestID);
			if(oldestObj->has_method("recycle_pooled_object"))
				oldestObj->call("recycle_pooled_object");
			return cast_to<Node>(oldestObj);
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
		return cast_to<Node>(returnObj);
	}

	Node* new_object = _sceneToInstantiate->instantiate();

	if (new_object->has_method("managed_by_pool")) {
		new_object->call("managed_by_pool", this);
	}
	_inUseObjects.push_back(new_object->get_instance_id());
	return new_object;
}

void ThreadedObjectPool::return_instance(Node *instance) {
	if(_is_clearing_instances)
		// instances might be inclined to return themselves while
		// we are in the process of clearing them!
		return;

	if(instance->get_parent() != nullptr) {
		instance->get_parent()->remove_child(instance);
	}
	_inUseObjects.erase(instance->get_instance_id());
	if(_availableObjects.find(instance->get_instance_id()) == -1)
		_availableObjects.push_back(instance->get_instance_id());
}

void ThreadedObjectPool::run_callbacks() {
	PROFILE_FUNCTION()
	MutexLock lock(_instanceListMutex);
	uint32_t i = 0;
	while(i < _finishedCreations.size()) {
		if(_finishedCreations[i].CreatedInstance != nullptr) {
			if(_finishedCreations[i].CreatedInstance->has_method("recycle_pooled_object"))
				_finishedCreations[i].CreatedInstance->call("recycle_pooled_object");
			_inUseObjects.push_back(_finishedCreations[i].CreatedInstance->get_instance_id());
			_finishedCreations[i].CreationCallback.call(_finishedCreations[i].CreatedInstance);
			_finishedCreations.remove_at_unordered(i);
		}
		else {
			++i;
		}
	}
	i = 0;
	while(i < _taskIDsInQueue.size()) {
		if(WorkerThreadPool::get_singleton()->is_task_completed(_taskIDsInQueue[i])) {
			WorkerThreadPool::get_singleton()->wait_for_task_completion(_taskIDsInQueue[i]);
			_taskIDsInQueue.remove_at_unordered(i);
		} else {
			i++;
		}
	}
}
void ThreadedObjectPool::clear_all_instances() {
	_is_clearing_instances = true;

	MutexLock lock(_instanceListMutex);
	while(_taskIDsInQueue.size() > 0) {
		WorkerThreadPool::get_singleton()->wait_for_task_completion(_taskIDsInQueue[0]);
		_taskIDsInQueue.remove_at_unordered(0);
	}

	for(auto& creationData : _finishedCreations) {
		if(creationData.CreatedInstance != nullptr)
			creationData.CreatedInstance->queue_free();
	}
	_instanceCreationQueue.clear();
	_finishedCreations.clear();

	for(ObjectID objId : _inUseObjects) {
		Node* obj = Object::cast_to<Node>(ObjectDB::get_instance(objId));
		if(obj == nullptr)
			continue;
		obj->queue_free();
	}
	_inUseObjects.clear();
	for(ObjectID objId : _availableObjects) {
		Node* obj = Object::cast_to<Node>(ObjectDB::get_instance(objId));
		if(obj == nullptr)
			continue;
		obj->queue_free();
	}
	_availableObjects.clear();

	_is_clearing_instances = false;
}
