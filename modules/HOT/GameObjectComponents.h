#ifndef GODOT_CLONE_GAMEOBJECTCOMPONENTS_H
#define GODOT_CLONE_GAMEOBJECTCOMPONENTS_H

#include "GameObject.h"
#include "ModifiedValues.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/physics_body_2d.h"
#include <scene/2d/node_2d.h>

class GameObjectComponent : public Node {
	GDCLASS(GameObjectComponent, Node)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_gameobject", "gameobject"), &GameObjectComponent::set_gameobject);
		ClassDB::bind_method(D_METHOD("get_gameobject"), &GameObjectComponent::get_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_gameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_gameobject", "get_gameobject");

		ClassDB::bind_method(D_METHOD("set_externalSource", "externalSource"), &GameObjectComponent::set_externalsource_gameobject);
		ClassDB::bind_method(D_METHOD("get_externalSource"), &GameObjectComponent::get_externalsource_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_externalSourceGameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_externalSource", "get_externalSource");

		ClassDB::bind_method(D_METHOD("set_position_provider", "positionProvider"), &GameObjectComponent::set_position_provider);
		ClassDB::bind_method(D_METHOD("get_position_provider"), &GameObjectComponent::get_position_provider);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_positionProvider", PROPERTY_HINT_RESOURCE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_position_provider", "get_position_provider");

		ClassDB::bind_method(D_METHOD("initGameObjectComponent"), &GameObjectComponent::init_gameobject_component);
		ClassDB::bind_method(D_METHOD("get_gameobjectWorldPosition"), &GameObjectComponent::get_gameobject_worldposition);
		ClassDB::bind_method(D_METHOD("createModifiedIntValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponent::create_modified_int_value, DEFVAL(Callable()));
		ClassDB::bind_method(D_METHOD("createModifiedFloatValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponent::create_modified_float_value, DEFVAL(Callable()));
	}

	ObjectID _gameObjectID;
	GameObject* _gameObject = nullptr;
	ObjectID _externalSourceGameObjectID;
	GameObject* _externalSourceGameObject = nullptr;
	ObjectID _positionProviderID;
	Node* _positionProvider = nullptr;

public:
	void init_gameobject_component() {
		_gameObject = GameObject::getGameObjectInParents(this);
		if(_gameObject == nullptr || _gameObject->is_queued_for_deletion()) {
			_gameObject = nullptr;
			_gameObjectID = ObjectID();
			return;
		}
		_gameObjectID = _gameObject->get_instance_id();
		_positionProvider = _gameObject->getChildNodeWithMethod("get_worldPosition");
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}

	Vector2 get_gameobject_worldposition() {
		if(_positionProviderID.is_valid() && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider->callv("get_worldPosition", {});
		return {};
	}

	void set_gameobject(GameObject* gameobject)  {
		_gameObject = gameobject;
		if(_gameObject != nullptr)
			_gameObjectID = _gameObject->get_instance_id();
		else
			_gameObjectID = ObjectID();
	}
	GameObject* get_gameobject() {
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			return _gameObject;
		return nullptr;
	}

	void set_externalsource_gameobject(GameObject *gameObject) {
		_externalSourceGameObject = gameObject;
		if(_externalSourceGameObject != nullptr)
			_externalSourceGameObjectID = _externalSourceGameObject->get_instance_id();
		else
			_externalSourceGameObjectID = ObjectID();
	}

	GameObject *get_externalsource_gameobject() {
		if(_externalSourceGameObject != nullptr && ObjectDB::get_instance(_externalSourceGameObjectID) != nullptr)
			return _externalSourceGameObject;
		return nullptr;
	}

	void set_position_provider(Node* positionProvider) {
		_positionProvider = positionProvider;
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}
	Node* get_position_provider() {
		if(_positionProvider != nullptr && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider;
		return nullptr;
	}

	Ref<ModifiedIntValue> create_modified_int_value(int baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedIntValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
	Ref<ModifiedFloatValue> create_modified_float_value(float baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedFloatValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
};



// the way the inheritance and the ObjectDB is set up in godot, we can't really
// do multiple versions of the GameObjectComponent with different base classes.
// that's why these are just copies with different names and base classes...
class GameObjectComponent2D : public Node2D {
	GDCLASS(GameObjectComponent2D, Node2D)
	
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_gameobject", "gameobject"), &GameObjectComponent2D::set_gameobject);
		ClassDB::bind_method(D_METHOD("get_gameobject"), &GameObjectComponent2D::get_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_gameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_gameobject", "get_gameobject");

		ClassDB::bind_method(D_METHOD("set_externalSource", "externalSource"), &GameObjectComponent2D::set_externalsource_gameobject);
		ClassDB::bind_method(D_METHOD("get_externalSource"), &GameObjectComponent2D::get_externalsource_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_externalSourceGameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_externalSource", "get_externalSource");

		ClassDB::bind_method(D_METHOD("set_position_provider", "positionProvider"), &GameObjectComponent2D::set_position_provider);
		ClassDB::bind_method(D_METHOD("get_position_provider"), &GameObjectComponent2D::get_position_provider);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_positionProvider", PROPERTY_HINT_RESOURCE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_position_provider", "get_position_provider");

		ClassDB::bind_method(D_METHOD("initGameObjectComponent"), &GameObjectComponent2D::init_gameobject_component);
		ClassDB::bind_method(D_METHOD("get_gameobjectWorldPosition"), &GameObjectComponent2D::get_gameobject_worldposition);
		ClassDB::bind_method(D_METHOD("createModifiedIntValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponent2D::create_modified_int_value, DEFVAL(Callable()));
		ClassDB::bind_method(D_METHOD("createModifiedFloatValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponent2D::create_modified_float_value, DEFVAL(Callable()));
	}

	ObjectID _gameObjectID;
	GameObject* _gameObject = nullptr;
	ObjectID _externalSourceGameObjectID;
	GameObject* _externalSourceGameObject = nullptr;
	ObjectID _positionProviderID;
	Node* _positionProvider = nullptr;

public:
	void init_gameobject_component() {
		_gameObject = GameObject::getGameObjectInParents(this);
		if(_gameObject == nullptr || _gameObject->is_queued_for_deletion()) {
			_gameObject = nullptr;
			_gameObjectID = ObjectID();
			return;
		}
		_gameObjectID = _gameObject->get_instance_id();
		_positionProvider = _gameObject->getChildNodeWithMethod("get_worldPosition");
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}

	Vector2 get_gameobject_worldposition() {
		if(_positionProviderID.is_valid() && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider->callv("get_worldPosition", {});
		return {};
	}

	void set_gameobject(GameObject* gameobject)  {
		_gameObject = gameobject;
		if(_gameObject != nullptr)
			_gameObjectID = _gameObject->get_instance_id();
		else
			_gameObjectID = ObjectID();
	}
	GameObject* get_gameobject() {
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			return _gameObject;
		return nullptr;
	}

	void set_externalsource_gameobject(GameObject *gameObject) {
		_externalSourceGameObject = gameObject;
		if(_externalSourceGameObject != nullptr)
			_externalSourceGameObjectID = _externalSourceGameObject->get_instance_id();
		else
			_externalSourceGameObjectID = ObjectID();
	}

	GameObject *get_externalsource_gameobject() {
		if(_externalSourceGameObject != nullptr && ObjectDB::get_instance(_externalSourceGameObjectID) != nullptr)
			return _externalSourceGameObject;
		return nullptr;
	}

	void set_position_provider(Node* positionProvider) {
		_positionProvider = positionProvider;
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}
	Node* get_position_provider() {
		if(_positionProvider != nullptr && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider;
		return nullptr;
	}

	Ref<ModifiedIntValue> create_modified_int_value(int baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedIntValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
	Ref<ModifiedFloatValue> create_modified_float_value(float baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedFloatValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
};



class GameObjectComponentArea2D : public Area2D {
	GDCLASS(GameObjectComponentArea2D, Area2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_gameobject", "gameobject"), &GameObjectComponentArea2D::set_gameobject);
		ClassDB::bind_method(D_METHOD("get_gameobject"), &GameObjectComponentArea2D::get_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_gameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_gameobject", "get_gameobject");

		ClassDB::bind_method(D_METHOD("set_externalSource", "externalSource"), &GameObjectComponentArea2D::set_externalsource_gameobject);
		ClassDB::bind_method(D_METHOD("get_externalSource"), &GameObjectComponentArea2D::get_externalsource_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_externalSourceGameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_externalSource", "get_externalSource");

		ClassDB::bind_method(D_METHOD("set_position_provider", "positionProvider"), &GameObjectComponentArea2D::set_position_provider);
		ClassDB::bind_method(D_METHOD("get_position_provider"), &GameObjectComponentArea2D::get_position_provider);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_positionProvider", PROPERTY_HINT_RESOURCE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_position_provider", "get_position_provider");

		ClassDB::bind_method(D_METHOD("initGameObjectComponent"), &GameObjectComponentArea2D::init_gameobject_component);
		ClassDB::bind_method(D_METHOD("get_gameobjectWorldPosition"), &GameObjectComponentArea2D::get_gameobject_worldposition);
		ClassDB::bind_method(D_METHOD("createModifiedIntValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentArea2D::create_modified_int_value, DEFVAL(Callable()));
		ClassDB::bind_method(D_METHOD("createModifiedFloatValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentArea2D::create_modified_float_value, DEFVAL(Callable()));
	}

	ObjectID _gameObjectID;
	GameObject* _gameObject = nullptr;
	ObjectID _externalSourceGameObjectID;
	GameObject* _externalSourceGameObject = nullptr;
	ObjectID _positionProviderID;
	Node* _positionProvider = nullptr;

public:
	void init_gameobject_component() {
		_gameObject = GameObject::getGameObjectInParents(this);
		if(_gameObject == nullptr || _gameObject->is_queued_for_deletion()) {
			_gameObject = nullptr;
			_gameObjectID = ObjectID();
			return;
		}
		_gameObjectID = _gameObject->get_instance_id();
		_positionProvider = _gameObject->getChildNodeWithMethod("get_worldPosition");
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}

	Vector2 get_gameobject_worldposition() {
		if(_positionProviderID.is_valid() && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider->callv("get_worldPosition", {});
		return {};
	}

	void set_gameobject(GameObject* gameobject)  {
		_gameObject = gameobject;
		if(_gameObject != nullptr)
			_gameObjectID = _gameObject->get_instance_id();
		else
			_gameObjectID = ObjectID();
	}
	GameObject* get_gameobject() {
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			return _gameObject;
		return nullptr;
	}

	void set_externalsource_gameobject(GameObject *gameObject) {
		_externalSourceGameObject = gameObject;
		if(_externalSourceGameObject != nullptr)
			_externalSourceGameObjectID = _externalSourceGameObject->get_instance_id();
		else
			_externalSourceGameObjectID = ObjectID();
	}

	GameObject *get_externalsource_gameobject() {
		if(_externalSourceGameObject != nullptr && ObjectDB::get_instance(_externalSourceGameObjectID) != nullptr)
			return _externalSourceGameObject;
		return nullptr;
	}

	void set_position_provider(Node* positionProvider) {
		_positionProvider = positionProvider;
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}
	Node* get_position_provider() {
		if(_positionProvider != nullptr && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider;
		return nullptr;
	}

	Ref<ModifiedIntValue> create_modified_int_value(int baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedIntValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
	Ref<ModifiedFloatValue> create_modified_float_value(float baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedFloatValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
};

class GameObjectComponentRigidBody2D : public RigidBody2D {
	GDCLASS(GameObjectComponentRigidBody2D, RigidBody2D)
	
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_gameobject", "gameobject"), &GameObjectComponentRigidBody2D::set_gameobject);
		ClassDB::bind_method(D_METHOD("get_gameobject"), &GameObjectComponentRigidBody2D::get_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_gameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_gameobject", "get_gameobject");

		ClassDB::bind_method(D_METHOD("set_externalSource", "externalSource"), &GameObjectComponentRigidBody2D::set_externalsource_gameobject);
		ClassDB::bind_method(D_METHOD("get_externalSource"), &GameObjectComponentRigidBody2D::get_externalsource_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_externalSourceGameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_externalSource", "get_externalSource");

		ClassDB::bind_method(D_METHOD("set_position_provider", "positionProvider"), &GameObjectComponentRigidBody2D::set_position_provider);
		ClassDB::bind_method(D_METHOD("get_position_provider"), &GameObjectComponentRigidBody2D::get_position_provider);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_positionProvider", PROPERTY_HINT_RESOURCE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_position_provider", "get_position_provider");

		ClassDB::bind_method(D_METHOD("initGameObjectComponent"), &GameObjectComponentRigidBody2D::init_gameobject_component);
		ClassDB::bind_method(D_METHOD("get_gameobjectWorldPosition"), &GameObjectComponentRigidBody2D::get_gameobject_worldposition);
		ClassDB::bind_method(D_METHOD("createModifiedIntValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentRigidBody2D::create_modified_int_value, DEFVAL(Callable()));
		ClassDB::bind_method(D_METHOD("createModifiedFloatValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentRigidBody2D::create_modified_float_value, DEFVAL(Callable()));
	}

	ObjectID _gameObjectID;
	GameObject* _gameObject = nullptr;
	ObjectID _externalSourceGameObjectID;
	GameObject* _externalSourceGameObject = nullptr;
	ObjectID _positionProviderID;
	Node* _positionProvider = nullptr;

public:
	void init_gameobject_component() {
		_gameObject = GameObject::getGameObjectInParents(this);
		if(_gameObject == nullptr || _gameObject->is_queued_for_deletion()) {
			_gameObject = nullptr;
			_gameObjectID = ObjectID();
			return;
		}
		_gameObjectID = _gameObject->get_instance_id();
		_positionProvider = _gameObject->getChildNodeWithMethod("get_worldPosition");
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}

	Vector2 get_gameobject_worldposition() {
		if(_positionProviderID.is_valid() && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider->callv("get_worldPosition", {});
		return {};
	}

	void set_gameobject(GameObject* gameobject)  {
		_gameObject = gameobject;
		if(_gameObject != nullptr)
			_gameObjectID = _gameObject->get_instance_id();
		else
			_gameObjectID = ObjectID();
	}
	GameObject* get_gameobject() {
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			return _gameObject;
		return nullptr;
	}

	void set_externalsource_gameobject(GameObject *gameObject) {
		_externalSourceGameObject = gameObject;
		if(_externalSourceGameObject != nullptr)
			_externalSourceGameObjectID = _externalSourceGameObject->get_instance_id();
		else
			_externalSourceGameObjectID = ObjectID();
	}

	GameObject *get_externalsource_gameobject() {
		if(_externalSourceGameObject != nullptr && ObjectDB::get_instance(_externalSourceGameObjectID) != nullptr)
			return _externalSourceGameObject;
		return nullptr;
	}

	void set_position_provider(Node* positionProvider) {
		_positionProvider = positionProvider;
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}
	Node* get_position_provider() {
		if(_positionProvider != nullptr && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider;
		return nullptr;
	}

	Ref<ModifiedIntValue> create_modified_int_value(int baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedIntValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
	Ref<ModifiedFloatValue> create_modified_float_value(float baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedFloatValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
};


class GameObjectComponentKinematicBody2D : public CharacterBody2D {
	GDCLASS(GameObjectComponentKinematicBody2D, CharacterBody2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_gameobject", "gameobject"), &GameObjectComponentKinematicBody2D::set_gameobject);
		ClassDB::bind_method(D_METHOD("get_gameobject"), &GameObjectComponentKinematicBody2D::get_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_gameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_gameobject", "get_gameobject");

		ClassDB::bind_method(D_METHOD("set_externalSource", "externalSource"), &GameObjectComponentKinematicBody2D::set_externalsource_gameobject);
		ClassDB::bind_method(D_METHOD("get_externalSource"), &GameObjectComponentKinematicBody2D::get_externalsource_gameobject);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_externalSourceGameObject", PROPERTY_HINT_RESOURCE_TYPE, "GameObject", PROPERTY_USAGE_NONE), "set_externalSource", "get_externalSource");

		ClassDB::bind_method(D_METHOD("set_position_provider", "positionProvider"), &GameObjectComponentKinematicBody2D::set_position_provider);
		ClassDB::bind_method(D_METHOD("get_position_provider"), &GameObjectComponentKinematicBody2D::get_position_provider);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_positionProvider", PROPERTY_HINT_RESOURCE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_position_provider", "get_position_provider");

		ClassDB::bind_method(D_METHOD("initGameObjectComponent"), &GameObjectComponentKinematicBody2D::init_gameobject_component);
		ClassDB::bind_method(D_METHOD("get_gameobjectWorldPosition"), &GameObjectComponentKinematicBody2D::get_gameobject_worldposition);
		ClassDB::bind_method(D_METHOD("createModifiedIntValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentKinematicBody2D::create_modified_int_value, DEFVAL(Callable()));
		ClassDB::bind_method(D_METHOD("createModifiedFloatValue", "baseVal", "modifierName", "rankModifier"), &GameObjectComponentKinematicBody2D::create_modified_float_value, DEFVAL(Callable()));
	}

	ObjectID _gameObjectID;
	GameObject* _gameObject = nullptr;
	ObjectID _externalSourceGameObjectID;
	GameObject* _externalSourceGameObject = nullptr;
	ObjectID _positionProviderID;
	Node* _positionProvider = nullptr;

public:
	void init_gameobject_component() {
		_gameObject = GameObject::getGameObjectInParents(this);
		if(_gameObject == nullptr || _gameObject->is_queued_for_deletion()) {
			_gameObject = nullptr;
			_gameObjectID = ObjectID();
			return;
		}
		_gameObjectID = _gameObject->get_instance_id();
		_positionProvider = _gameObject->getChildNodeWithMethod("get_worldPosition");
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}

	Vector2 get_gameobject_worldposition() {
		if(_positionProviderID.is_valid() && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider->callv("get_worldPosition", {});
		return {};
	}

	void set_gameobject(GameObject* gameobject)  {
		_gameObject = gameobject;
		if(_gameObject != nullptr)
			_gameObjectID = _gameObject->get_instance_id();
		else
			_gameObjectID = ObjectID();
	}
	GameObject* get_gameobject() {
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			return _gameObject;
		return nullptr;
	}

	void set_externalsource_gameobject(GameObject *gameObject) {
		_externalSourceGameObject = gameObject;
		if(_externalSourceGameObject != nullptr)
			_externalSourceGameObjectID = _externalSourceGameObject->get_instance_id();
		else
			_externalSourceGameObjectID = ObjectID();
	}

	GameObject *get_externalsource_gameobject() {
		if(_externalSourceGameObject != nullptr && ObjectDB::get_instance(_externalSourceGameObjectID) != nullptr)
			return _externalSourceGameObject;
		return nullptr;
	}

	void set_position_provider(Node* positionProvider) {
		_positionProvider = positionProvider;
		if(_positionProvider != nullptr)
			_positionProviderID = _positionProvider->get_instance_id();
		else
			_positionProviderID = ObjectID();
	}
	Node* get_position_provider() {
		if(_positionProvider != nullptr && ObjectDB::get_instance(_positionProviderID) != nullptr)
			return _positionProvider;
		return nullptr;
	}

	Ref<ModifiedIntValue> create_modified_int_value(int baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedIntValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
	Ref<ModifiedFloatValue> create_modified_float_value(float baseVal, String modifierName, Callable rankModifier) {
		Ref<ModifiedFloatValue> modValue;
		modValue.instantiate();
		if(_gameObject != nullptr && ObjectDB::get_instance(_gameObjectID) != nullptr)
			modValue->_init(baseVal, modifierName, _gameObject, rankModifier);
		return modValue;
	}
};

#endif //GODOT_CLONE_GAMEOBJECTCOMPONENTS_H
