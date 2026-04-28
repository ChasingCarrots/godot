#include "blendalot_state_machine.h"

#include "core/object/class_db.h"

//
// BLTStateMachineTransition
//
void BLTStateMachineTransition::_bind_methods() {
	ClassDB::bind_method(D_METHOD("force_transition", "flag"), &BLTStateMachineTransition::force_transition);
}

void BLTStateMachineTransition::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::FLOAT, transition_duration_name, PROPERTY_HINT_RANGE, "0,1,0.01,or_less,or_greater"));
}

bool BLTStateMachineTransition::_get(const StringName &p_name, Variant &r_value) const {
	String prop_name = p_name;
	if (prop_name == transition_duration_name) {
		r_value = transition_duration;
		return true;
	}

	return false;
}

bool BLTStateMachineTransition::_set(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name == transition_duration_name) {
		transition_duration = p_value;
		return true;
	}

	return false;
}

//
// BLTStateMachine
//
void BLTStateMachine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_state", "state"), &BLTStateMachine::add_state);
	ClassDB::bind_method(D_METHOD("remove_state", "state"), &BLTStateMachine::remove_state);
	ClassDB::bind_method(D_METHOD("get_state", "state_name"), &BLTStateMachine::get_state);
	ClassDB::bind_method(D_METHOD("get_state_names"), &BLTStateMachine::get_state_names_as_typed_array);

	ClassDB::bind_method(D_METHOD("is_transition_valid", "from_state", "to_state"), &BLTStateMachine::is_transition_valid);
	ClassDB::bind_method(D_METHOD("add_transition", "from_state", "to_state", "transition"), &BLTStateMachine::add_transition);
	ClassDB::bind_method(D_METHOD("remove_transition", "from_state", "to_state"), &BLTStateMachine::remove_transition);
	ClassDB::bind_method(D_METHOD("get_transitions"), &BLTStateMachine::get_transitions_as_array);

	ClassDB::bind_method(D_METHOD("set_graph_offset", "graph_offset"), &BLTStateMachine::set_graph_offset);
	ClassDB::bind_method(D_METHOD("get_graph_offset"), &BLTStateMachine::get_graph_offset);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "graph_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_graph_offset", "get_graph_offset");

	BIND_CONSTANT(TRANSITION_OK);
	BIND_CONSTANT(TRANSITION_ERROR_NO_FROM_STATE);
	BIND_CONSTANT(TRANSITION_ERROR_NO_TO_STATE);
	BIND_CONSTANT(TRANSITION_ERROR_ALREADY_EXISTS);
}

void BLTStateMachine::get_parameter_list(List<PropertyInfo> *p_list) const {
	for (const Ref<BLTStateMachineTransition> &transition : transitions) {
		String parameter_name = vformat("transition/%s/activate", transition->get_name());
		p_list->push_back(PropertyInfo(Variant::BOOL, parameter_name));
	}
}

void BLTStateMachine::set_parameter(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name.begins_with("transition/")) {
		String transition_name = prop_name.get_slicec('/', 1);
		String transition_parameter = prop_name.get_slicec('/', 2);
		int transition_index = find_transition_index_by_name(transition_name);

		if (transition_parameter == "activate") {
			if (transition_index != -1) {
				transitions[transition_index]->force_transition(p_value);
				return;
			}
		}
	}
}

Variant BLTStateMachine::get_parameter(const StringName &p_name) const {
	String prop_name = p_name;
	if (prop_name.begins_with("transition/")) {
		String transition_name = prop_name.get_slicec('/', 1);
		String transition_parameter = prop_name.get_slicec('/', 2);
		int transition_index = find_transition_index_by_name(transition_name);

		if (transition_parameter == "activate") {
			if (transition_index != -1) {
				return transitions[transition_index]->is_transition_forced;
			}
		}
	}

	return Variant();
}

Variant BLTStateMachine::get_parameter_default_value(const StringName &p_parameter) const {
	String prop_name = p_parameter;
	if (prop_name.begins_with("transition/")) {
		String transition_name = prop_name.get_slicec('/', 1);
		String transition_parameter = prop_name.get_slicec('/', 2);
		int transition_index = find_transition_index_by_name(transition_name);

		if (transition_parameter == "activate") {
			if (transition_index != -1) {
				return false;
			}
		}
	}

	return Variant();
}

void BLTStateMachine::_get_property_list(List<PropertyInfo> *p_list) const {
	for (const Ref<BLTAnimationNode> &state : states) {
		String prop_name = state->get_name();
		p_list->push_back(PropertyInfo(Variant::OBJECT, "states/" + prop_name + "/node", PROPERTY_HINT_RESOURCE_TYPE, "AnimationNode", PROPERTY_USAGE_NO_EDITOR));
		p_list->push_back(PropertyInfo(Variant::VECTOR2, "states/" + prop_name + "/position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
	}

	p_list->push_back(PropertyInfo(Variant::ARRAY, "transitions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
	p_list->push_back(PropertyInfo(Variant::VECTOR2, "graph_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
}

bool BLTStateMachine::_get(const StringName &p_name, Variant &r_value) const {
	String prop_name = p_name;
	if (prop_name.begins_with("states/")) {
		String state_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);
		int state_index = find_state_index_by_name(state_name);

		if (what == "node") {
			if (state_index != -1) {
				r_value = states[state_index];
				return true;
			}
		}

		if (what == "position") {
			if (state_index != -1) {
				r_value = states[state_index]->position;
				return true;
			}
		}
	} else if (prop_name == "transitions") {
		Array conns;
		conns.resize(transitions.size() * 3);

		for (uint32_t i = 0; i < transitions.size(); i++) {
			conns[i * 3 + 0] = transition_states[i][0];
			conns[i * 3 + 1] = transition_states[i][1];
			conns[i * 3 + 2] = transitions[i];
		}

		r_value = conns;
		return true;
	} else if (prop_name == "graph_offset") {
		r_value = graph_offset;
		return true;
	}

	return false;
}

bool BLTStateMachine::_set(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name.begins_with("states/")) {
		String state_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);

		if (what == "node") {
			Ref<BLTAnimationNode> anode = p_value;
			if (anode.is_valid()) {
				anode->set_name(state_name);
				add_state(anode);
			}
			return true;
		}

		if (what == "position") {
			int state_index = find_state_index_by_name(state_name);
			if (state_index > -1) {
				states[state_index]->position = p_value;
			}
			return true;
		}
	} else if (prop_name == "transitions") {
		Array transitions = p_value;
		ERR_FAIL_COND_V(transitions.size() % 3 != 0, false);

		bool success = true;
		for (int i = 0; i < transitions.size(); i += 3) {
			if (!add_transition(transitions[i], transitions[i + 1], transitions[i + 2])) {
				success = false;
			}
		}

		return success;
	} else if (prop_name == "graph_offset") {
		graph_offset = p_value;
		return true;
	}

	return false;
}