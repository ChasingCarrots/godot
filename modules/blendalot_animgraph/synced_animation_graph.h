#pragma once

#include "scene/animation/animation_player.h"
#include "synced_animation_node.h"

#include <cassert>

class Skeleton3D;

class SyncedAnimationGraph : public Node {
	GDCLASS(SyncedAnimationGraph, Node);

private:
	NodePath animation_player_path;
	Ref<SyncedAnimationNode> root_animation_node;
	NodePath skeleton_path;

	GraphEvaluationContext graph_context = {};
	AnimationData graph_output;

	mutable List<PropertyInfo> properties;
	mutable AHashMap<StringName, Pair<Ref<SyncedAnimationNode>, StringName>> parameter_to_node_parameter_map;

	mutable bool properties_dirty = true;

	void _update_properties() const;
	void _update_properties_for_node(const String &p_base_path, Ref<SyncedAnimationNode> p_node) const;

	void _tree_changed();

protected:
	void _notification(int p_what);
	static void _bind_methods();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	/* ---- General settings for animation ---- */
	AnimationMixer::AnimationCallbackModeProcess callback_mode_process = AnimationMixer::ANIMATION_CALLBACK_MODE_PROCESS_IDLE;
	AnimationMixer::AnimationCallbackModeMethod callback_mode_method = AnimationMixer::ANIMATION_CALLBACK_MODE_METHOD_DEFERRED;
	AnimationMixer::AnimationCallbackModeDiscrete callback_mode_discrete = AnimationMixer::ANIMATION_CALLBACK_MODE_DISCRETE_RECESSIVE;

	bool processing = false;
	bool active = true;

public:
	void _process_graph(double p_delta, bool p_update_only = false);
	void _apply_animation_data(const AnimationData &output_data) const;

	void set_active(bool p_active);
	bool is_active() const;

	void set_animation_player(const NodePath &p_path);
	NodePath get_animation_player() const;

	void set_root_animation_node(const Ref<SyncedAnimationNode> &p_animation_node);
	Ref<SyncedAnimationNode> get_root_animation_node() const;

	void set_skeleton(const NodePath &p_path);
	NodePath get_skeleton() const;

	void set_callback_mode_process(AnimationMixer::AnimationCallbackModeProcess p_mode);
	AnimationMixer::AnimationCallbackModeProcess get_callback_mode_process() const;

	void set_callback_mode_method(AnimationMixer::AnimationCallbackModeMethod p_mode);
	AnimationMixer::AnimationCallbackModeMethod get_callback_mode_method() const;

	void set_callback_mode_discrete(AnimationMixer::AnimationCallbackModeDiscrete p_mode);
	AnimationMixer::AnimationCallbackModeDiscrete get_callback_mode_discrete() const;

	GraphEvaluationContext &get_context() {
		return graph_context;
	}

	SyncedAnimationGraph();

private:
	void _set_process(bool p_process, bool p_force = false);

	void _setup_evaluation_context();
	void _cleanup_evaluation_context();

	void _setup_graph();
};
