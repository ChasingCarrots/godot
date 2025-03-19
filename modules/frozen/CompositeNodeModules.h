#ifndef COMPOSITENODEMODULES_H
#define COMPOSITENODEMODULES_H

#include "CompositeNode.h"
#include "CompositeNodeValue.h"

#include <scene/3d/physics/character_body_3d.h>

/////////// NOTE: Changes to the CompositeNodeModule class should also be done to the LiveTemplate
///////////       and you have to recreate all the additional CompositeNodeModule classes from that LiveTemplate.
///////////       (The way Godot inheritance works, there doesn't seem to be any other way!)

// CompositeNodeModule    Node
class CompositeNodeModule : public Node {
	GDCLASS(CompositeNodeModule, Node)
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_composite_node"),
				&CompositeNodeModule::get_composite_node);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_composite_node", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
				"", "get_composite_node");

		ClassDB::bind_method(D_METHOD("create_non_synchronized_value", "value_name", "initial_value"),
				&CompositeNodeModule::create_non_synchronized_value, DEFVAL(Variant()));
		ClassDB::bind_method(D_METHOD("create_synchronized_value", "value_name", "initial_value", "sync_mode", "sync_type"),
				&CompositeNodeModule::create_synchronized_value);
		ClassDB::bind_method(D_METHOD("create_synchronized_array", "value_name"),
				&CompositeNodeModule::create_synchronized_array);

		ClassDB::bind_method(D_METHOD("register_callback", "callback_name", "callable"),
			&CompositeNodeModule::register_callback);
		ClassDB::bind_method(D_METHOD("register_function", "function_name", "callable"),
			&CompositeNodeModule::register_function);
		ClassDB::bind_method(D_METHOD("register_data_updated_callback", "data_name", "callable"),
			&CompositeNodeModule::register_data_updated_callback);

		GDVIRTUAL_BIND(_ready_composite_node);
		GDVIRTUAL_BIND(_ready_authority);
	}

	void _notification(int p_notification) {
		switch (p_notification) {
			case NOTIFICATION_READY:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_ready();
				}
				break;
			case NOTIFICATION_EXIT_TREE:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_exit_tree();
				}
				break;
		}
	}
	void _ready() {
		_composite_node = CompositeNode::GetCompositeNodeInParents(this);
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), vformat("CompositeNodeModule has to be a child of a CompositeNode. (%s)", get_name()));
		_composite_node->RegisterCallback("init_authority", callable_mp(this, &CompositeNodeModule::init_authority));

		GDVIRTUAL_CALL(_ready_composite_node);
	}
	void _exit_tree() {
		if (_composite_node.is_valid()) {
			_composite_node->UnregisterCallback("init_authority", callable_mp(this, &CompositeNodeModule::init_authority));
			for (const auto &named_callable : RegisteredCallbacks) {
				_composite_node->UnregisterCallback(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredFunctions) {
				_composite_node->UnregisterFunction(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredDataUpdates) {
				_composite_node->UnregisterDataUpdatedCallback(named_callable.Name, named_callable.CallableObj);
			}
		}
	}

	SafeObjectPointer<CompositeNode> _composite_node;

	struct NamedCallable {
		StringName Name;
		Callable CallableObj;
	};
	Vector<NamedCallable> RegisteredCallbacks;
	Vector<NamedCallable> RegisteredFunctions;
	Vector<NamedCallable> RegisteredDataUpdates;

	void init_authority(int player_id) {
		GDVIRTUAL_CALL(_ready_authority);
	}

public:
	GDVIRTUAL0(_ready_composite_node)
	GDVIRTUAL0(_ready_authority)

	Ref<CompositeNodeValue> create_non_synchronized_value(StringName value_name, Variant initial_value) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule::create_non_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_non_synchronized(_composite_node.get_nocheck(), value_name, initial_value);
	}
	Ref<CompositeNodeValue> create_synchronized_value(StringName value_name, Variant initial_value,
			CompositeNode::DataSynchronizationMode mode,
			CompositeNode::DataSynchronizationType type) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule::create_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_synchronized(_composite_node.get_nocheck(), value_name, initial_value, mode, type);
	}
	Ref<SynchronizedArray> create_synchronized_array(StringName variable_name) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule::create_synchronized_array called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return SynchronizedArray::create(_composite_node->GetCommunicationLine(), variable_name);
	}
	void register_callback(StringName callback_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule::register_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredCallbacks.append({ callback_name, callable });
		_composite_node->RegisterCallback(callback_name, callable);
	}
	void register_function(StringName function_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule::register_function called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredFunctions.append({ function_name, callable });
		_composite_node->RegisterFunction(function_name, callable);
	}
	void register_data_updated_callback(StringName data_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule::register_data_updated_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredDataUpdates.append({ data_name, callable });
		_composite_node->RegisterDataUpdatedCallback(data_name, callable);
	}
	[[nodiscard]] CompositeNode *get_composite_node() const { return _composite_node.get_nocheck(); }
};


// CompositeNodeModule3D    Node3D
class CompositeNodeModule3D : public Node3D {
	GDCLASS(CompositeNodeModule3D, Node3D)
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_composite_node"),
				&CompositeNodeModule3D::get_composite_node);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_composite_node", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
				"", "get_composite_node");

		ClassDB::bind_method(D_METHOD("create_non_synchronized_value", "value_name", "initial_value"),
				&CompositeNodeModule3D::create_non_synchronized_value, DEFVAL(Variant()));
		ClassDB::bind_method(D_METHOD("create_synchronized_value", "value_name", "initial_value", "sync_mode", "sync_type"),
				&CompositeNodeModule3D::create_synchronized_value);
		ClassDB::bind_method(D_METHOD("create_synchronized_array", "value_name"),
				&CompositeNodeModule3D::create_synchronized_array);

		ClassDB::bind_method(D_METHOD("register_callback", "callback_name", "callable"),
			&CompositeNodeModule3D::register_callback);
		ClassDB::bind_method(D_METHOD("register_function", "function_name", "callable"),
			&CompositeNodeModule3D::register_function);
		ClassDB::bind_method(D_METHOD("register_data_updated_callback", "data_name", "callable"),
			&CompositeNodeModule3D::register_data_updated_callback);

		GDVIRTUAL_BIND(_ready_composite_node);
		GDVIRTUAL_BIND(_ready_authority);
	}

	void _notification(int p_notification) {
		switch (p_notification) {
			case NOTIFICATION_READY:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_ready();
				}
				break;
			case NOTIFICATION_EXIT_TREE:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_exit_tree();
				}
				break;
		}
	}
	void _ready() {
		_composite_node = CompositeNode::GetCompositeNodeInParents(this);
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), vformat("CompositeNodeModule3D has to be a child of a CompositeNode. (%s)", get_name()));
		_composite_node->RegisterCallback("init_authority", callable_mp(this, &CompositeNodeModule3D::init_authority));

		GDVIRTUAL_CALL(_ready_composite_node);
	}
	void _exit_tree() {
		if (_composite_node.is_valid()) {
			_composite_node->UnregisterCallback("init_authority", callable_mp(this, &CompositeNodeModule3D::init_authority));
			for (const auto &named_callable : RegisteredCallbacks) {
				_composite_node->UnregisterCallback(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredFunctions) {
				_composite_node->UnregisterFunction(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredDataUpdates) {
				_composite_node->UnregisterDataUpdatedCallback(named_callable.Name, named_callable.CallableObj);
			}
		}
	}

	SafeObjectPointer<CompositeNode> _composite_node;

	struct NamedCallable {
		StringName Name;
		Callable CallableObj;
	};
	Vector<NamedCallable> RegisteredCallbacks;
	Vector<NamedCallable> RegisteredFunctions;
	Vector<NamedCallable> RegisteredDataUpdates;

	void init_authority(int player_id) {
		GDVIRTUAL_CALL(_ready_authority);
	}

public:
	GDVIRTUAL0(_ready_composite_node)
	GDVIRTUAL0(_ready_authority)

	Ref<CompositeNodeValue> create_non_synchronized_value(StringName value_name, Variant initial_value) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule3D::create_non_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_non_synchronized(_composite_node.get_nocheck(), value_name, initial_value);
	}
	Ref<CompositeNodeValue> create_synchronized_value(StringName value_name, Variant initial_value,
			CompositeNode::DataSynchronizationMode mode,
			CompositeNode::DataSynchronizationType type) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule3D::create_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_synchronized(_composite_node.get_nocheck(), value_name, initial_value, mode, type);
	}
	Ref<SynchronizedArray> create_synchronized_array(StringName variable_name) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModule3D::create_synchronized_array called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return SynchronizedArray::create(_composite_node->GetCommunicationLine(), variable_name);
	}
	void register_callback(StringName callback_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule3D::register_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredCallbacks.append({ callback_name, callable });
		_composite_node->RegisterCallback(callback_name, callable);
	}
	void register_function(StringName function_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule3D::register_function called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredFunctions.append({ function_name, callable });
		_composite_node->RegisterFunction(function_name, callable);
	}
	void register_data_updated_callback(StringName data_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModule3D::register_data_updated_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredDataUpdates.append({ data_name, callable });
		_composite_node->RegisterDataUpdatedCallback(data_name, callable);
	}
	[[nodiscard]] CompositeNode *get_composite_node() const { return _composite_node.get_nocheck(); }
};


// CompositeNodeModuleCharacterBody3D    CharacterBody3D
class CompositeNodeModuleCharacterBody3D : public CharacterBody3D {
	GDCLASS(CompositeNodeModuleCharacterBody3D, CharacterBody3D)
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_composite_node"),
				&CompositeNodeModuleCharacterBody3D::get_composite_node);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_composite_node", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
				"", "get_composite_node");

		ClassDB::bind_method(D_METHOD("create_non_synchronized_value", "value_name", "initial_value"),
				&CompositeNodeModuleCharacterBody3D::create_non_synchronized_value, DEFVAL(Variant()));
		ClassDB::bind_method(D_METHOD("create_synchronized_value", "value_name", "initial_value", "sync_mode", "sync_type"),
				&CompositeNodeModuleCharacterBody3D::create_synchronized_value);
		ClassDB::bind_method(D_METHOD("create_synchronized_array", "value_name"),
				&CompositeNodeModuleCharacterBody3D::create_synchronized_array);

		ClassDB::bind_method(D_METHOD("register_callback", "callback_name", "callable"),
			&CompositeNodeModuleCharacterBody3D::register_callback);
		ClassDB::bind_method(D_METHOD("register_function", "function_name", "callable"),
			&CompositeNodeModuleCharacterBody3D::register_function);
		ClassDB::bind_method(D_METHOD("register_data_updated_callback", "data_name", "callable"),
			&CompositeNodeModuleCharacterBody3D::register_data_updated_callback);

		GDVIRTUAL_BIND(_ready_composite_node);
		GDVIRTUAL_BIND(_ready_authority);
	}

	void _notification(int p_notification) {
		switch (p_notification) {
			case NOTIFICATION_READY:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_ready();
				}
				break;
			case NOTIFICATION_EXIT_TREE:
				if (!Engine::get_singleton()->is_editor_hint()) {
					_exit_tree();
				}
				break;
		}
	}
	void _ready() {
		_composite_node = CompositeNode::GetCompositeNodeInParents(this);
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), vformat("CompositeNodeModuleCharacterBody3D has to be a child of a CompositeNode. (%s)", get_name()));
		_composite_node->RegisterCallback("init_authority", callable_mp(this, &CompositeNodeModuleCharacterBody3D::init_authority));

		GDVIRTUAL_CALL(_ready_composite_node);
	}
	void _exit_tree() {
		if (_composite_node.is_valid()) {
			_composite_node->UnregisterCallback("init_authority", callable_mp(this, &CompositeNodeModuleCharacterBody3D::init_authority));
			for (const auto &named_callable : RegisteredCallbacks) {
				_composite_node->UnregisterCallback(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredFunctions) {
				_composite_node->UnregisterFunction(named_callable.Name, named_callable.CallableObj);
			}
			for (const auto &named_callable : RegisteredDataUpdates) {
				_composite_node->UnregisterDataUpdatedCallback(named_callable.Name, named_callable.CallableObj);
			}
		}
	}

	SafeObjectPointer<CompositeNode> _composite_node;

	struct NamedCallable {
		StringName Name;
		Callable CallableObj;
	};
	Vector<NamedCallable> RegisteredCallbacks;
	Vector<NamedCallable> RegisteredFunctions;
	Vector<NamedCallable> RegisteredDataUpdates;

	void init_authority(int player_id) {
		GDVIRTUAL_CALL(_ready_authority);
	}

public:
	GDVIRTUAL0(_ready_composite_node)
	GDVIRTUAL0(_ready_authority)

	Ref<CompositeNodeValue> create_non_synchronized_value(StringName value_name, Variant initial_value) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModuleCharacterBody3D::create_non_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_non_synchronized(_composite_node.get_nocheck(), value_name, initial_value);
	}
	Ref<CompositeNodeValue> create_synchronized_value(StringName value_name, Variant initial_value,
			CompositeNode::DataSynchronizationMode mode,
			CompositeNode::DataSynchronizationType type) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModuleCharacterBody3D::create_synchronized_value called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return CompositeNodeValue::create_synchronized(_composite_node.get_nocheck(), value_name, initial_value, mode, type);
	}
	Ref<SynchronizedArray> create_synchronized_array(StringName variable_name) {
		ERR_FAIL_COND_V_MSG(!_composite_node.is_valid(), {}, "CompositeNodeModuleCharacterBody3D::create_synchronized_array called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		return SynchronizedArray::create(_composite_node->GetCommunicationLine(), variable_name);
	}
	void register_callback(StringName callback_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModuleCharacterBody3D::register_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredCallbacks.append({ callback_name, callable });
		_composite_node->RegisterCallback(callback_name, callable);
	}
	void register_function(StringName function_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModuleCharacterBody3D::register_function called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredFunctions.append({ function_name, callable });
		_composite_node->RegisterFunction(function_name, callable);
	}
	void register_data_updated_callback(StringName data_name, Callable callable) {
		ERR_FAIL_COND_MSG(!_composite_node.is_valid(), "CompositeNodeModuleCharacterBody3D::register_data_updated_callback called with no composite node set. Called too early? Use the _ready_composite_node virtual function.");
		RegisteredDataUpdates.append({ data_name, callable });
		_composite_node->RegisterDataUpdatedCallback(data_name, callable);
	}
	[[nodiscard]] CompositeNode *get_composite_node() const { return _composite_node.get_nocheck(); }
};

#endif //COMPOSITENODEMODULES_H
