#include "synced_animation_graph.h"

#include "core/os/time.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"

void SyncedAnimationGraph::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_active", "active"), &SyncedAnimationGraph::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &SyncedAnimationGraph::is_active);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

	ClassDB::bind_method(D_METHOD("set_callback_mode_process", "mode"), &SyncedAnimationGraph::set_callback_mode_process);
	ClassDB::bind_method(D_METHOD("get_callback_mode_process"), &SyncedAnimationGraph::get_callback_mode_process);

	ClassDB::bind_method(D_METHOD("set_callback_mode_method", "mode"), &SyncedAnimationGraph::set_callback_mode_method);
	ClassDB::bind_method(D_METHOD("get_callback_mode_method"), &SyncedAnimationGraph::get_callback_mode_method);

	ClassDB::bind_method(D_METHOD("set_animation_player", "animation_player"), &SyncedAnimationGraph::set_animation_player);
	ClassDB::bind_method(D_METHOD("get_animation_player"), &SyncedAnimationGraph::get_animation_player);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "animation_player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "AnimationPlayer"), "set_animation_player", "get_animation_player");
	ADD_SIGNAL(MethodInfo(SNAME("animation_player_changed")));

	ClassDB::bind_method(D_METHOD("set_tree_root", "animation_node"), &SyncedAnimationGraph::set_root_animation_node);
	ClassDB::bind_method(D_METHOD("get_tree_root"), &SyncedAnimationGraph::get_root_animation_node);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tree_root", PROPERTY_HINT_RESOURCE_TYPE, "SyncedAnimationNode"), "set_tree_root", "get_tree_root");

	ClassDB::bind_method(D_METHOD("set_skeleton", "skeleton"), &SyncedAnimationGraph::set_skeleton);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &SyncedAnimationGraph::get_skeleton);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton", "get_skeleton");
	ADD_SIGNAL(MethodInfo(SNAME("skeleton_changed")));
}

void SyncedAnimationGraph::_update_properties_for_node(const String &p_base_path, Ref<SyncedAnimationNode> p_node) const {
	ERR_FAIL_COND(p_node.is_null());

	List<PropertyInfo> plist;
	p_node->get_parameter_list(&plist);
	for (PropertyInfo &pinfo : plist) {
		StringName key = pinfo.name;

		if (!parameter_to_node_parameter_map.has(p_base_path + key)) {
			parameter_to_node_parameter_map[p_base_path + key] = Pair<Ref<SyncedAnimationNode>, StringName>(p_node, key);
		}

		pinfo.name = p_base_path + key;
		properties.push_back(pinfo);
	}

	List<Ref<SyncedAnimationNode>> children;
	p_node->get_child_nodes(&children);

	for (const Ref<SyncedAnimationNode> &child_node : children) {
		_update_properties_for_node(p_base_path + child_node->name + "/", child_node);
	}
}

void SyncedAnimationGraph::_update_properties() const {
	if (!properties_dirty) {
		return;
	}

	properties.clear();
	parameter_to_node_parameter_map.clear();

	if (root_animation_node.is_valid()) {
		_update_properties_for_node(Animation::PARAMETERS_BASE_PATH, root_animation_node);
	}

	properties_dirty = false;

	const_cast<SyncedAnimationGraph *>(this)->notify_property_list_changed();
}

bool SyncedAnimationGraph::_set(const StringName &p_name, const Variant &p_value) {
#ifndef DISABLE_DEPRECATED
	String name = p_name;
	if (name == "process_callback") {
		set_callback_mode_process(static_cast<AnimationMixer::AnimationCallbackModeProcess>((int)p_value));
		return true;
	}
#endif // DISABLE_DEPRECATED
	if (properties_dirty) {
		_update_properties();
	}

	if (parameter_to_node_parameter_map.has(p_name)) {
		const Pair<Ref<SyncedAnimationNode>, StringName> &property_node = parameter_to_node_parameter_map[p_name];
		if (!property_node.first.is_valid()) {
			print_error(vformat("Cannot set property '%s' node not found.", p_name));
			return false;
		}

		property_node.first->set_parameter(property_node.second, p_value);
		return true;
	}

	return false;
}

bool SyncedAnimationGraph::_get(const StringName &p_name, Variant &r_ret) const {
#ifndef DISABLE_DEPRECATED
	if (p_name == "process_callback") {
		r_ret = get_callback_mode_process();
		return true;
	}
#endif // DISABLE_DEPRECATED
	if (properties_dirty) {
		_update_properties();
	}

	if (parameter_to_node_parameter_map.has(p_name)) {
		const Pair<Ref<SyncedAnimationNode>, StringName> &property_node = parameter_to_node_parameter_map[p_name];
		if (!property_node.first.is_valid()) {
			print_error(vformat("Cannot get property '%s' node not found.", p_name));
			return false;
		}
		r_ret = property_node.first->get_parameter(property_node.second);
		return true;
	}

	return false;
}

void SyncedAnimationGraph::_get_property_list(List<PropertyInfo> *p_list) const {
	if (properties_dirty) {
		_update_properties();
	}

	for (const PropertyInfo &E : properties) {
		p_list->push_back(E);
	}
}

void SyncedAnimationGraph::_tree_changed() {
	if (properties_dirty) {
		return;
	}

	callable_mp(this, &SyncedAnimationGraph::_update_properties).call_deferred();
	properties_dirty = true;
}

void SyncedAnimationGraph::_notification(int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_READY: {
			_setup_evaluation_context();
			_setup_graph();

			if (active) {
				_set_process(true);
			}
		} break;

		case Node::NOTIFICATION_INTERNAL_PROCESS: {
			if (active) {
				_process_graph(get_process_delta_time());
			}
		} break;

		case Node::NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (active) {
				_process_graph(get_physics_process_delta_time());
			}
		} break;

		case Node::NOTIFICATION_EXIT_TREE: {
			_cleanup_evaluation_context();
			break;
		}

		default: {
			break;
		}
	}
}

void SyncedAnimationGraph::set_active(bool p_active) {
	if (active == p_active) {
		return;
	}

	active = p_active;
	_set_process(processing, true);
}

bool SyncedAnimationGraph::is_active() const {
	return active;
}

void SyncedAnimationGraph::set_callback_mode_process(AnimationMixer::AnimationCallbackModeProcess p_mode) {
	if (callback_mode_process == p_mode) {
		return;
	}

	bool was_active = is_active();
	if (was_active) {
		set_active(false);
	}

	callback_mode_process = p_mode;

	if (was_active) {
		set_active(true);
	}
}

AnimationMixer::AnimationCallbackModeProcess SyncedAnimationGraph::get_callback_mode_process() const {
	return callback_mode_process;
}

void SyncedAnimationGraph::set_callback_mode_method(AnimationMixer::AnimationCallbackModeMethod p_mode) {
	callback_mode_method = p_mode;
	emit_signal(SNAME("mixer_updated"));
}

AnimationMixer::AnimationCallbackModeMethod SyncedAnimationGraph::get_callback_mode_method() const {
	return callback_mode_method;
}

void SyncedAnimationGraph::set_callback_mode_discrete(AnimationMixer::AnimationCallbackModeDiscrete p_mode) {
	callback_mode_discrete = p_mode;
	emit_signal(SNAME("mixer_updated"));
}

AnimationMixer::AnimationCallbackModeDiscrete SyncedAnimationGraph::get_callback_mode_discrete() const {
	return callback_mode_discrete;
}

void SyncedAnimationGraph::set_animation_player(const NodePath &p_path) {
	animation_player_path = p_path;
	if (p_path.is_empty()) {
		//		set_root_node(SceneStringName(path_pp));
		//		while (animation_libraries.size()) {
		//			remove_animation_library(animation_libraries[0].name);
		//		}
	}
	graph_context.animation_player = Object::cast_to<AnimationPlayer>(get_node_or_null(animation_player_path));

	_setup_evaluation_context();
	_setup_graph();

	emit_signal(SNAME("animation_player_changed")); // Needs to unpin AnimationPlayerEditor.
}

NodePath SyncedAnimationGraph::get_animation_player() const {
	return animation_player_path;
}

void SyncedAnimationGraph::set_root_animation_node(const Ref<SyncedAnimationNode> &p_animation_node) {
	if (root_animation_node.is_valid()) {
		root_animation_node->disconnect(SNAME("tree_changed"), callable_mp(this, &SyncedAnimationGraph::_tree_changed));
	}

	root_animation_node = p_animation_node;

	if (root_animation_node.is_valid()) {
		_setup_graph();
		root_animation_node->connect(SNAME("tree_changed"), callable_mp(this, &SyncedAnimationGraph::_tree_changed));
	}

	properties_dirty = true;

	update_configuration_warnings();
}

Ref<SyncedAnimationNode> SyncedAnimationGraph::get_root_animation_node() const {
	return root_animation_node;
}

void SyncedAnimationGraph::set_skeleton(const NodePath &p_path) {
	skeleton_path = p_path;
	if (p_path.is_empty()) {
		//		set_root_node(SceneStringName(path_pp));
		//		while (animation_libraries.size()) {
		//			remove_animation_library(animation_libraries[0].name);
		//		}
	}
	graph_context.skeleton_3d = Object::cast_to<Skeleton3D>(get_node_or_null(skeleton_path));

	_setup_evaluation_context();
	_setup_graph();

	emit_signal(SNAME("skeleton_changed")); // Needs to unpin AnimationPlayerEditor.
}

NodePath SyncedAnimationGraph::get_skeleton() const {
	return skeleton_path;
}

void SyncedAnimationGraph::_process_graph(double p_delta, bool p_update_only) {
	if (!root_animation_node.is_valid()) {
		return;
	}

	_update_properties();

	root_animation_node->activate_inputs(Vector<Ref<SyncedAnimationNode>>());
	root_animation_node->calculate_sync_track(Vector<Ref<SyncedAnimationNode>>());
	root_animation_node->update_time(p_delta);
	root_animation_node->evaluate(graph_context, LocalVector<AnimationData *>(), graph_output);

	_apply_animation_data(graph_output);
}

void SyncedAnimationGraph::_apply_animation_data(const AnimationData &output_data) const {
	for (const KeyValue<Animation::TypeHash, AnimationData::TrackValue *> &K : output_data.track_values) {
		const AnimationData::TrackValue *track_value = K.value;
		switch (track_value->type) {
			case AnimationData::TrackType::TYPE_POSITION_3D:
			case AnimationData::TrackType::TYPE_ROTATION_3D: {
				const AnimationData::TransformTrackValue *transform_track_value = static_cast<const AnimationData::TransformTrackValue *>(track_value);

				int bone_idx = -1;
				NodePath path = transform_track_value->track->path;

				if (path.get_subname_count() == 1) {
					bone_idx = graph_context.skeleton_3d->find_bone(path.get_subname(0));

					if (bone_idx != -1) {
						if (transform_track_value->loc_used) {
							graph_context.skeleton_3d->set_bone_pose_position(transform_track_value->bone_idx, transform_track_value->loc);
						}

						if (transform_track_value->rot_used) {
							graph_context.skeleton_3d->set_bone_pose_rotation(transform_track_value->bone_idx, transform_track_value->rot);
						}
					} else {
						assert(false && "Not yet implemented!");
					}
				}

				break;
			}
			default: {
				print_line(vformat("Unsupported track type %d", track_value->type));
				break;
			}
		}
	}

	graph_context.skeleton_3d->force_update_all_bone_transforms();
}

void SyncedAnimationGraph::_set_process(bool p_process, bool p_force) {
	if (processing == p_process && !p_force) {
		return;
	}

	set_physics_process_internal(false);
	set_process_internal(true);

	processing = p_process;
}

void SyncedAnimationGraph::_setup_evaluation_context() {
	_cleanup_evaluation_context();

	graph_context.animation_player = Object::cast_to<AnimationPlayer>(get_node_or_null(animation_player_path));
	graph_context.skeleton_3d = Object::cast_to<Skeleton3D>(get_node_or_null(skeleton_path));
}

void SyncedAnimationGraph::_cleanup_evaluation_context() {
	graph_context.animation_player = nullptr;
	graph_context.skeleton_3d = nullptr;
}

void SyncedAnimationGraph::_setup_graph() {
	if (graph_context.animation_player == nullptr || graph_context.skeleton_3d == nullptr || !root_animation_node.is_valid()) {
		return;
	}

	root_animation_node->initialize(graph_context);
}

SyncedAnimationGraph::SyncedAnimationGraph() {
}
