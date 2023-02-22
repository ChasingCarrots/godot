#ifndef HOTEXTENSION_GAMEOBJECT_H
#define HOTEXTENSION_GAMEOBJECT_H

#include <scene/2d/node_2d.h>

#include <vector>

struct SignalCallable {
	String SignalName;
	Callable CallableConnection;
};

class GameObject : public Node2D {
    GDCLASS(GameObject, Node2D)

	Variant _spawn_origin;
	GameObject* _inheritModifierFrom = nullptr;
	std::vector<SignalCallable> _connectedSignals;
	std::vector<Variant> _sourceTree;

	std::vector<Node*> _tempNodeArray;
	void populateTempNodesWithAllChildren();

protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods();
	void _notification(int p_notification);

public:
    // calls from godot
    void _ready();
    void _exit_tree();
    void _child_entered_tree(Node* childNode);

    // collective signal system
    void connectToSignal(StringName signalName, Callable callable);
    void disconnectFromSignal(StringName signalName, Callable callable);
    bool hasSignal(StringName signalName);
    void injectEmitSignal(StringName signalName, Array parameters);

    // collective child nodes system
    Node* getChildNodeWithMethod(StringName methodName);
    Node* getChildNodeWithSignal(StringName signalName);
    void getChildNodesWithMethod(StringName methodName, Array fillArray);
	Node* getChildNodeInGroup(StringName groupName);

	// modifier system
	void setInheritModifierFrom(GameObject* otherGameObject);
	void triggerModifierUpdated(StringName modifierType);
	Variant calculateModifiedValue(StringName modifierType, Variant baseValue, TypedArray<String> categories);
	GameObject* getInheritModifierFrom();

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
