#ifndef COMPOSITENODEVALUE_H
#define COMPOSITENODEVALUE_H

#include "CompositeNode.h"

class CompositeNodeValue : public RefCounted {
    GDCLASS(CompositeNodeValue, RefCounted)
protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods() {
        ClassDB::bind_static_method("CompositeNodeValue", D_METHOD("create_non_synchronized", "on_node", "value_name", "initial_value"),
            &CompositeNodeValue::create_non_synchronized, DEFVAL(Variant()));
        ClassDB::bind_static_method("CompositeNodeValue", D_METHOD("create_synchronized", "on_node", "value_name", "initial_value", "mode", "type"),
            &CompositeNodeValue::create_synchronized);

        ClassDB::bind_method(D_METHOD("set_value", "new_value"), &CompositeNodeValue::set_value);
        ClassDB::bind_method(D_METHOD("get_value"), &CompositeNodeValue::get_value);
        ADD_PROPERTY(PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_value", "get_value");

        ADD_SIGNAL(MethodInfo("ValueChanged", PropertyInfo(Variant::NIL, "NewValue")));
    }

    SafeObjectPointer<CompositeNode> _composite_node;
    StringName _value_name;
    Variant _current_value;
    bool _is_synchronized = false;

    void init_authority(int player_id) {
        if (_current_value.get_type() != Variant::NIL && _composite_node->is_multiplayer_authority()) {
            // we have to do a little shuffle, so that set_value calls the callbacks and does the things
            Variant initial_value = _current_value;
            _current_value = Variant();
            set_value(initial_value);
        }
    }
    void value_updated(Variant new_value) {
        _current_value = new_value;
        emit_signal("ValueChanged", _current_value);
    }
public:
	~CompositeNodeValue() override {
        if (_composite_node.is_valid()) {
            _composite_node->UnregisterDataUpdatedCallback(_value_name, callable_mp(this, &CompositeNodeValue::value_updated));
            _composite_node->UnregisterCallback("init_authority", callable_mp(this, &CompositeNodeValue::init_authority));
        }
	    _composite_node.set(nullptr);
    }

    static Ref<CompositeNodeValue> create_non_synchronized(CompositeNode *on_node, StringName value_name, Variant initial_value) {
	    Ref<CompositeNodeValue> new_value;
        new_value.instantiate();
        new_value->_is_synchronized = false;
        new_value->_value_name = value_name;
        new_value->_current_value = initial_value;
        new_value->_composite_node = on_node;
        on_node->RegisterDataUpdatedCallback(value_name, callable_mp(new_value.ptr(), &CompositeNodeValue::value_updated), true);
        if (initial_value.get_type() != Variant::NIL) {
            on_node->RegisterCallback("init_authority", callable_mp(new_value.ptr(), &CompositeNodeValue::init_authority));
        }
        return new_value;
    }
    static Ref<CompositeNodeValue> create_synchronized(CompositeNode *on_node,
        StringName value_name, Variant initial_value,
        CompositeNode::DataSynchronizationMode mode,
        CompositeNode::DataSynchronizationType type)
    {
        Ref<CompositeNodeValue> new_value;
        new_value.instantiate();
	    new_value->_is_synchronized = true;
        new_value->_value_name = value_name;
        new_value->_current_value = initial_value;
        new_value->_composite_node = on_node;
        on_node->SetupDataMultiplayerSynchronization(value_name, mode, type);
        on_node->RegisterDataUpdatedCallback(value_name, callable_mp(new_value.ptr(), &CompositeNodeValue::value_updated), true);
        if (initial_value.get_type() != Variant::NIL) {
            on_node->RegisterCallback("init_authority", callable_mp(new_value.ptr(), &CompositeNodeValue::init_authority));
        }
        return new_value;
    }

    void set_value(Variant value) {
	    if (_current_value == value) {
	        return;
	    }
        _current_value = value;
        if (_composite_node.is_valid()) {
            if (_is_synchronized) _composite_node->SetDataOnAuthority(_value_name, _current_value);
            else _composite_node->SetData(_value_name, _current_value);
        }
    }
    Variant get_value() const {
        return _current_value;
    }
};



#endif //COMPOSITENODEVALUE_H
