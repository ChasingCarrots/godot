#ifndef GODOT_CLONE_SAFEOBJECTPOINTER_H
#define GODOT_CLONE_SAFEOBJECTPOINTER_H

#include <core/object/object.h>

template<typename Type>
class SafeObjectPointer {
public:
	void set(Type* pointedObject) {
		if(pointedObject != nullptr) {
			_pointedObject = pointedObject;
			_objectID = pointedObject->get_instance_id();
		}
		else {
			_pointedObject = nullptr;
			_objectID = ObjectID();
		}
	}

	bool is_valid() const {
		return _pointedObject != nullptr && ObjectDB::get_instance(_objectID) != nullptr;
	}

	Type* get() {
		if(_pointedObject != nullptr) {
			if(ObjectDB::get_instance(_objectID) != nullptr)
				return _pointedObject;
			_pointedObject = nullptr;
		}
		return nullptr;
	}

	Type* get_nocheck() { return _pointedObject; }

	explicit operator Type*() { return get(); }
	Type* operator=(Type* pointedObject) { set(pointedObject); return pointedObject; }
	Type* operator->() { return _pointedObject; }
private:
	Type* _pointedObject = nullptr;
	ObjectID _objectID;
};

#endif //GODOT_CLONE_SAFEOBJECTPOINTER_H
