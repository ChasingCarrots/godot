#pragma once

#include "blendalot_animation_node.h"

#include "core/object/callable_mp.h"

class BLTStateMachineTransition : public BLTAnimationNodeBlend2 {
	GDCLASS(BLTStateMachineTransition, BLTAnimationNodeBlend2);

	StringName transition_duration_name = PNAME("transition_duration");

	friend class BLTStateMachine;

	bool is_transition_forced = false;
	double transition_time = 0.f;
	double transition_duration = 0.1;

protected:
	static void _bind_methods();

	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	void force_transition(bool value) {
		is_transition_forced = value;
	}

	void set_transition_duration(double value) {
		transition_duration = value;
	}

	double get_transition_duration() const {
		return transition_duration;
	}

	double get_transition_time() const {
		return transition_time;
	}

	bool evaluate_condition() {
		if (is_transition_forced) {
			is_transition_forced = false;
			return true;
		}

		return false;
	}

	void reset() {
		transition_time = 0.f;
		is_transition_forced = false;
		// TODO: We need to set it to LOOP_LINEAR here so that update_time() properly updates the sync position. Does this semantically make sense?
		node_time_info.loop_mode = Animation::LOOP_LINEAR;
	}

	void update_transition_time_and_weight(double p_delta) {
		transition_time = CLAMP(transition_time + p_delta, 0., transition_duration);
		blend_weight = transition_time / transition_duration;
	}

	bool initialize(GraphEvaluationContext &context) override {
		reset();

		if (!BLTAnimationNode::initialize(context)) {
			return false;
		}

		return true;
	}
};

class BLTStateMachine : public BLTAnimationNode {
	GDCLASS(BLTStateMachine, BLTAnimationNode);

public:
	enum TransitionError {
		TRANSITION_OK,
		TRANSITION_ERROR_NO_FROM_STATE,
		TRANSITION_ERROR_NO_TO_STATE,
		TRANSITION_ERROR_ALREADY_EXISTS
	};

private:
	GraphEvaluationContext *_graph_evaluation_context = nullptr;

	LocalVector<Ref<BLTAnimationNode>> states;
	LocalVector<Ref<BLTStateMachineTransition>> transitions;

	// must be sorted by priority
	LocalVector<LocalVector<Ref<BLTStateMachineTransition>>> state_leaving_transitions;
	typedef LocalVector<Ref<BLTAnimationNode>> TransitionStatePair;
	LocalVector<TransitionStatePair> transition_states;

	Ref<BLTAnimationNode> entry_state;
	Ref<BLTAnimationNode> active_state;
	Ref<BLTAnimationNode> previous_state;
	int active_state_index = -1;

	Ref<BLTStateMachineTransition> active_transition;
	int active_transition_index = -1;

	void activate_state(const Ref<BLTAnimationNode> &state) {
		active_state_index = states.find(state);
		assert(active_state_index != -1);

		active_state = state;
	}

	void activate_transition(const Ref<BLTStateMachineTransition> &transition) {
		transition->reset();
		active_transition = transition;
		active_transition_index = transitions.find(transition);

		previous_state = active_state;
		activate_state(transition_states[active_transition_index][1]);

		if (node_time_info.is_synced) {
			active_transition->node_time_info.sync_position = node_time_info.sync_position;
			active_transition->node_time_info.is_synced = true;
		} else if (active_transition->sync && previous_state.is_valid()) {
			double previous_sync_position = previous_state->node_time_info.sync_track.calc_sync_from_abs_time(previous_state->node_time_info.position);
			active_transition->node_time_info.sync_position = previous_sync_position;
		}
	}

	void deactivate_transition(const Ref<BLTStateMachineTransition> &transition) {
		if (active_transition != transition) {
			return;
		}

		previous_state = nullptr;
		active_transition = nullptr;

		// A transition may have marked an input as synced. The node_time_info is unchanged by the
		// transition and can be used as a backup value.
		active_state->node_time_info.is_synced = node_time_info.is_synced;
	}

	void find_and_activate_transition() {
		for (Ref<BLTStateMachineTransition> &transition : state_leaving_transitions[active_state_index]) {
			if (transition->evaluate_condition()) {
				activate_transition(transition);
				return;
			}
		}
	}

protected:
	static void _bind_methods();

	void get_parameter_list(List<PropertyInfo> *p_list) const override;
	Variant get_parameter_default_value(const StringName &p_parameter) const override;
	void set_parameter(const StringName &p_name, const Variant &p_value) override;
	Variant get_parameter(const StringName &p_name) const override;

	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;

public:
	Vector2 graph_offset;

	Vector2 get_graph_offset() const {
		return graph_offset;
	}

	void set_graph_offset(const Vector2 &p_graph_offset) {
		graph_offset = p_graph_offset;
	}

	void _state_machine_changed(const StringName &node_name) {
		_node_changed();
	}

	int64_t find_state_index(const Ref<BLTAnimationNode> &state) const {
		return states.find(state);
	}

	int64_t find_state_index_by_name(const StringName &name) const {
		for (uint32_t i = 0; i < states.size(); i++) {
			if (states[i]->get_name() == name) {
				return i;
			}
		}
		return -1;
	}

	Ref<BLTStateMachineTransition> get_transition_by_index(int64_t index) const {
		if (index < 0 || index >= transitions.size()) {
			return nullptr;
		}

		return transitions[index];
	}

	Ref<BLTStateMachineTransition> get_active_transition() const {
		return active_transition;
	}

	int get_active_state_index() const {
		return active_state_index;
	}

	Ref<BLTAnimationNode> get_active_state() const {
		return active_state;
	}

	Ref<BLTAnimationNode> get_previous_state() const {
		return previous_state;
	}

	Ref<BLTAnimationNode> get_entry_state() const {
		return entry_state;
	}

	void add_state(const Ref<BLTAnimationNode> &state) {
		if (state == this) {
			print_error(vformat("Error: cannot add state machine %s to itself!", state->node_path));
			return;
		}

		StringName node_base_name = state->get_name();
		if (node_base_name.is_empty()) {
			node_base_name = state->get_class_name();
		}
		state->set_name(node_base_name);

		int number_suffix = 1;
		while (find_state_index_by_name(state->get_name()) != -1) {
			state->set_name(vformat("%s %d", node_base_name, number_suffix));
			number_suffix++;
		}
		state->node_path = vformat("%s/%s", node_path, state->get_name());

		states.push_back(state);
		state_leaving_transitions.push_back(LocalVector<Ref<BLTStateMachineTransition>>());

		if (_graph_evaluation_context != nullptr) {
			state->initialize(*_graph_evaluation_context);
		}

		if (entry_state.is_null()) {
			entry_state = state;
			_node_changed();
		}

		state->connect(SNAME("node_changed"), callable_mp(this, &BLTStateMachine::_state_machine_changed));
	}

	void remove_state(const Ref<BLTAnimationNode> &state) {
		int state_index = find_state_index(state);
		if (state_index == -1) {
			print_error(vformat("Cannot delete state %s: state not found.", state->get_name()));
			return;
		}

		// remove all transitions involving specified state
		unsigned int transition_index = transition_states.size();
		while (transition_index > 0) {
			transition_index--;
			const TransitionStatePair &transition_state_pair = transition_states[transition_index];
			if (transition_state_pair[0] == state || transition_state_pair[1] == state) {
				remove_transition(transition_state_pair[0], transition_state_pair[1]);
			}
		}

		states.remove_at(state_index);

		if (entry_state == state) {
			if (states.size() > 0) {
				entry_state = states[0];
			} else {
				entry_state = nullptr;
			}
		}

		if (active_state == state && entry_state.is_valid()) {
			activate_state(entry_state);
		}

		_node_changed();
	}

	TypedArray<StringName> get_state_names_as_typed_array() const {
		Vector<StringName> vec;
		for (const Ref<BLTAnimationNode> &state : states) {
			vec.push_back(state->get_name());
		}

		TypedArray<StringName> typed_arr;
		typed_arr.resize(vec.size());
		for (uint32_t i = 0; i < vec.size(); i++) {
			typed_arr[i] = vec[i];
		}
		return typed_arr;
	}

	Ref<BLTAnimationNode> get_state(const StringName &state_name) const {
		int state_index = find_state_index_by_name(state_name);

		if (state_index >= 0) {
			return states[state_index];
		}

		return nullptr;
	}

	TransitionError is_transition_valid(const Ref<BLTAnimationNode> &from_state, const Ref<BLTAnimationNode> &to_state) const {
		int64_t from_state_index = find_state_index(from_state);
		if (from_state_index == -1) {
			print_error(vformat("Cannot add transition from %s to %s: from state not found in StateMachine.", from_state->get_name(), to_state->get_name()));
			return TRANSITION_ERROR_NO_FROM_STATE;
		}

		if (find_state_index(to_state) == -1) {
			print_error(vformat("Cannot add transition from %s to %s: to state not found in StateMachine.", from_state->get_name(), to_state->get_name()));
			return TRANSITION_ERROR_NO_TO_STATE;
		}

		if (find_transition_index(from_state, to_state) != -1) {
			print_error(vformat("Cannot add transition from %s to %s: transition already exists.", from_state->get_name(), to_state->get_name()));
			return TRANSITION_ERROR_ALREADY_EXISTS;
		}

		return TRANSITION_OK;
	}

	TransitionError add_transition(const Ref<BLTAnimationNode> &from_state, const Ref<BLTAnimationNode> &to_state, const Ref<BLTStateMachineTransition> &transition) {
		const int64_t from_state_index = find_state_index(from_state);

		transition->set_name(vformat("%s_to_%s", from_state->get_name(), to_state->get_name()));

		const TransitionError transition_error = is_transition_valid(from_state, to_state);
		if (transition_error != TRANSITION_OK) {
			return transition_error;
		}

		if (transitions.find(transition) != -1) {
			print_error(vformat("Cannot add transition: transition already added."));
			return TRANSITION_ERROR_ALREADY_EXISTS;
		}

		transitions.push_back(transition);
		transition_states.push_back({ from_state, to_state });

		state_leaving_transitions[from_state_index].push_back(transition);

		_node_changed();

		return TRANSITION_OK;
	}

	void remove_transition(const Ref<BLTAnimationNode> &from_state, const Ref<BLTAnimationNode> &to_state) {
		int transition_index = -1;
		for (unsigned int i = 0; i < transition_states.size(); i++) {
			const TransitionStatePair &from_to_states = transition_states[i];
			if (from_to_states[0] == from_state && from_to_states[1] == to_state) {
				transition_index = i;
				break;
			}
		}

		if (transition_index == -1) {
			return;
		}

		if (active_transition_index == transition_index || active_transition == transitions[transition_index]) {
			print_error(vformat("Cannot delete transition %s -> %s: transition is active!", from_state->get_name(), to_state->get_name()));
			return;
		}

		int from_state_index = find_state_index(from_state);
		state_leaving_transitions[from_state_index].erase(transitions[transition_index]);

		transition_states.remove_at(transition_index);
		transitions.remove_at(transition_index);

		_node_changed();
	}

	int64_t find_transition_index(const Ref<BLTAnimationNode> &from_state, const Ref<BLTAnimationNode> &to_state) const {
		for (unsigned int i = 0; i < transition_states.size(); i++) {
			if (transition_states[i][0] == from_state && transition_states[i][1] == to_state) {
				return i;
			}
		}

		return -1;
	}

	int64_t find_transition_index_by_name(const String &transition_name) const {
		for (unsigned int i = 0; i < transitions.size(); i++) {
			if (transitions[i]->get_name() == transition_name) {
				return i;
			}
		}

		return -1;
	}

	Array get_transitions_as_array() const {
		Array result;

		for (uint32_t i = 0; i < transitions.size(); i++) {
			result.push_back(transition_states[i][0]);
			result.push_back(transition_states[i][1]);
			result.push_back(transitions[i]);
		}

		return result;
	}

	// overrides from BLTAnimationNode
	bool initialize(GraphEvaluationContext &context) override {
		GodotProfileZone("BLTStateMachine::initialize");

		_graph_evaluation_context = &context;

		bool has_failed_state = false;
		for (Ref<BLTAnimationNode> state : states) {
			state->node_path = vformat("%s/%s", node_path, state->get_name());
			if (!state->initialize(context)) {
				has_failed_state = true;
			}
		}

		active_transition = nullptr;
		active_state_index = -1;
		active_transition_index = -1;

		if (entry_state.is_null()) {
			context.validation_messages.push_back(vformat("Invalid node %s: No valid entry state defined.", node_path));
			return false;
		}

		return !has_failed_state;
	}

	void
	activate_inputs(const LocalVector<Ref<BLTAnimationNode>> &input_nodes) override {
		GodotProfileZone("BLTStateMachine::activate_inputs");

		if (active_state_index == -1) {
			activate_state(entry_state);
		}

		// Interruption is (not yet) allowed.
		if (!active_transition.is_valid()) {
			find_and_activate_transition();
		}

		if (active_transition.is_valid()) {
			active_transition->update_transition_time_and_weight(_graph_evaluation_context->graph_process_delta_time);
			active_transition->activate_inputs(transition_states[active_transition_index]);

			if (previous_state->active) {
				previous_state->activate_inputs({});
			}
			if (active_state->active) {
				active_state->activate_inputs({});
			}
		} else {
			active_state->active = true;
			active_state->node_time_info.is_synced = node_time_info.is_synced;
			active_state->activate_inputs({});
		}
	}

	void calculate_sync_track(const LocalVector<Ref<BLTAnimationNode>> &input_nodes) override {
		GodotProfileZone("BLTStateMachine::calculate_sync_track");

		if (active_transition.is_valid()) {
			if (previous_state->active) {
				previous_state->calculate_sync_track({});
			}

			if (active_state->active) {
				active_state->calculate_sync_track({});
			}

			active_transition->calculate_sync_track(transition_states[active_transition_index]);
			node_time_info.sync_track = active_transition->node_time_info.sync_track;
		} else {
			active_state->calculate_sync_track({});
			node_time_info.sync_track = active_state->node_time_info.sync_track;
		}
	}

	void update_time(double p_delta) override {
		GodotProfileZone("BLTStateMachine::update_time");

		BLTAnimationNode::update_time(p_delta);

		if (active_transition.is_valid()) {
			active_transition->update_time(p_delta);

			double delta;
			if (!node_time_info.is_synced && !active_transition->sync) {
				delta = active_transition->node_time_info.delta;
			} else {
				delta = active_transition->node_time_info.sync_position;
			}

			if (previous_state->active) {
				previous_state->update_time(delta);
			}

			if (active_state->active) {
				active_state->update_time(delta);
			}
		} else {
			active_state->update_time(p_delta);
		}
	}

	void evaluate(GraphEvaluationContext &context, const LocalVector<AnimationData *> &input_datas, AnimationData &output_data) override {
		GodotProfileZone("BLTStateMachine::evaluate");

		if (active_transition.is_valid()) {
			AnimationData *active_state_data = nullptr;
			AnimationData *previous_state_data = nullptr;

			if (previous_state->active) {
				previous_state_data = context.animation_data_allocator.allocate();
				previous_state->evaluate(context, LocalVector<AnimationData *>(), *previous_state_data);
			}

			if (active_state->active) {
				active_state_data = context.animation_data_allocator.allocate();
				active_state->evaluate(context, LocalVector<AnimationData *>(), *active_state_data);
			}

			active_transition->evaluate(context, { previous_state_data, active_state_data }, output_data);

			if (previous_state->active) {
				context.animation_data_allocator.free(previous_state_data);
				previous_state->active = false;
			}

			if (active_state->active) {
				context.animation_data_allocator.free(active_state_data);
				active_state->active = false;
			}

			// Deactivate any finished transitions
			if (Math::is_zero_approx(1. - active_transition->blend_weight)) {
				deactivate_transition(active_transition);
			}
		} else {
			active_state->evaluate(context, input_datas, output_data);
			active_state->active = false;
		}
	}
};

VARIANT_ENUM_CAST(BLTStateMachine::TransitionError)