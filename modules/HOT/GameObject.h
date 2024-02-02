#ifndef HOTEXTENSION_GAMEOBJECT_H
#define HOTEXTENSION_GAMEOBJECT_H

#include <scene/2d/node_2d.h>

#include <vector>

#include "Modifier.h"

#include <core/profiling.h>
#include <core/templates/oa_hash_map.h>
#include <scene/main/window.h>

struct SignalCallable {
	String SignalName;
	Callable CallableConnection;
};

class GameObject : public Node2D {
    GDCLASS(GameObject, Node2D)

	Variant _spawn_origin;
	Variant _inheritModifierFrom;
	GameObject* getValidatedInheritModifierFrom() {
		if (_inheritModifierFrom.get_type() != Variant::OBJECT)
			return nullptr;
		Object* gameObjectAsPtr = _inheritModifierFrom.get_validated_object();
		if (gameObjectAsPtr == nullptr)
			return nullptr;
		if (!gameObjectAsPtr->is_class("GameObject"))
			return nullptr;
		return static_cast<GameObject*>(gameObjectAsPtr);
	}
	LocalVector<SignalCallable> _connectedSignals;
	LocalVector<Variant> _sourceTree;
	LocalVector<Modifier*> _modifier;

	LocalVector<Node*> _tempNodeArray;
	void populateTempNodesWithAllChildren();

	OAHashMap<StringName, ObjectID> _childNodeWithMethodOrPropertyCache;

	static Node* _global;
	static Node* _world;
	static void InvalidateWorld() { _world = nullptr; }
protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods();
	void _notification(int p_notification);

public:
	inline static Node* Global() {
		if(_global == nullptr) {
			_global = SceneTree::get_singleton()->get_root()->get_node(NodePath("Global"));
			if(_global != nullptr)
				_global->connect("WorldRemoved", callable_mp_static(&GameObject::InvalidateWorld));
		}
		return _global;
	}
	inline static Node* World() {
		if(_world != nullptr)
			return _world;
		Variant worldVariant = Global()->get("World");
		if (worldVariant.is_null()) {
			print_error("Global.World is null!");
			return nullptr;
		}
		_world = cast_to<Node>(worldVariant);
		return _world;
	}

    void _connect_child_entered_tree();
    void _exit_tree();
    void _child_entered_tree(Node* childNode);

	static inline GameObject* getGameObjectInParents(Node* node) {
		PROFILE_FUNCTION();
		while(true) {
			if(node == nullptr) return nullptr;
			GameObject* currentNodeGO = dynamic_cast<GameObject*>(node);
			if(currentNodeGO != nullptr)
				return currentNodeGO;
			node = node->get_parent();
		}
	}

    // collective signal system
    void connectToSignal(String signalName, Callable callable);
    void disconnectFromSignal(String signalName, Callable callable);
    bool hasSignal(String signalName);
    void injectEmitSignal(String signalName, Array parameters);

    // collective child nodes system
    Node* getChildNodeWithMethod(const StringName& methodName);
    Node* getChildNodeWithSignal(const StringName& signalName);
	Node* getChildNodeWithProperty(const StringName& propertyName);
    void getChildNodesWithMethod(const StringName& methodName, Array fillArray);
	Node* getChildNodeInGroup(const StringName& groupName);

	// modifier system
	void setInheritModifierFrom(GameObject* otherGameObject, bool automaticallyKeepUpdated = false);
	void triggerModifierUpdated(String modifierType);
	Variant calculateModifiedValue(String modifierType, Variant baseValue, TypedArray<String> categories);
	float getAdditiveModifier(String modifierType, TypedArray<String> categories);
	float getMultiplicativeModifier(String modifierType, TypedArray<String> categories);
	GameObject* getInheritModifierFrom();
	void registerModifier(Modifier* modifier);
	void unregisterModifier(Modifier* modifier);
	Array getModifiers(String modifierType, TypedArray<String> categories);

	// effect system
	Node* add_effect(Node* effectScene, GameObject* externalSource);
	Node* find_effect(String effectID);

	// sourcetree system
	GameObject* get_rootSourceGameObject();
	void set_sourceGameObject(GameObject* source);

	// spawn origin
	void set_spawn_origin(Node* spawnOrigin);
	Node* get_spawn_origin();
};


#endif //HOTEXTENSION_GAMEOBJECT_H
