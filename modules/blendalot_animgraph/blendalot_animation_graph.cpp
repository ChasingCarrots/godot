#include "blendalot_animation_graph.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/profiling/profiling.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

void BLTAnimationGraph::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_active", "active"), &BLTAnimationGraph::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &BLTAnimationGraph::is_active);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

	ClassDB::bind_method(D_METHOD("set_callback_mode_process", "mode"), &BLTAnimationGraph::set_callback_mode_process);
	ClassDB::bind_method(D_METHOD("get_callback_mode_process"), &BLTAnimationGraph::get_callback_mode_process);

	ClassDB::bind_method(D_METHOD("set_callback_mode_method", "mode"), &BLTAnimationGraph::set_callback_mode_method);
	ClassDB::bind_method(D_METHOD("get_callback_mode_method"), &BLTAnimationGraph::get_callback_mode_method);

	ClassDB::bind_method(D_METHOD("set_animation_player", "animation_player"), &BLTAnimationGraph::set_animation_player);
	ClassDB::bind_method(D_METHOD("get_animation_player"), &BLTAnimationGraph::get_animation_player);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "animation_player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "AnimationPlayer"), "set_animation_player", "get_animation_player");
	ADD_SIGNAL(MethodInfo(SNAME("animation_player_changed")));

	ClassDB::bind_method(D_METHOD("set_tree_root", "animation_node"), &BLTAnimationGraph::set_root_animation_node);
	ClassDB::bind_method(D_METHOD("get_tree_root"), &BLTAnimationGraph::get_root_animation_node);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tree_root", PROPERTY_HINT_RESOURCE_TYPE, "BLTAnimationNode"), "set_tree_root", "get_tree_root");

	ClassDB::bind_method(D_METHOD("set_skeleton", "skeleton"), &BLTAnimationGraph::set_skeleton);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &BLTAnimationGraph::get_skeleton);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton", "get_skeleton");
	ADD_SIGNAL(MethodInfo(SNAME("skeleton_changed")));
}

void BLTAnimationGraph::_update_properties_for_node(const String &p_base_path, Ref<BLTAnimationNode> p_node) const {
	ERR_FAIL_COND(p_node.is_null());

	List<PropertyInfo> plist;
	p_node->get_parameter_list(&plist);
	for (PropertyInfo &pinfo : plist) {
		StringName key = pinfo.name;

		if (!parameter_to_node_parameter_map.has(p_base_path + key)) {
			parameter_to_node_parameter_map[p_base_path + key] = Pair<Ref<BLTAnimationNode>, StringName>(p_node, key);
		}

		pinfo.name = p_base_path + key;
		properties.push_back(pinfo);
	}

	List<Ref<BLTAnimationNode>> children;
	p_node->get_child_nodes(&children);

	for (const Ref<BLTAnimationNode> &child_node : children) {
		_update_properties_for_node(p_base_path + child_node->get_name() + "/", child_node);
	}
}

void BLTAnimationGraph::_update_properties() const {
	if (!properties_dirty) {
		return;
	}

	properties.clear();
	parameter_to_node_parameter_map.clear();

	if (root_animation_node.is_valid()) {
		_update_properties_for_node(Animation::PARAMETERS_BASE_PATH, root_animation_node);
	}

	properties_dirty = false;

	const_cast<BLTAnimationGraph *>(this)->notify_property_list_changed();
}

bool BLTAnimationGraph::_set(const StringName &p_name, const Variant &p_value) {
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
		const Pair<Ref<BLTAnimationNode>, StringName> &property_node = parameter_to_node_parameter_map[p_name];
		if (!property_node.first.is_valid()) {
			print_error(vformat("Cannot set property '%s' node not found.", p_name));
			return false;
		}

		property_node.first->set_parameter(property_node.second, p_value);
		return true;
	}

	return false;
}

bool BLTAnimationGraph::_get(const StringName &p_name, Variant &r_ret) const {
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
		const Pair<Ref<BLTAnimationNode>, StringName> &property_node = parameter_to_node_parameter_map[p_name];
		if (!property_node.first.is_valid()) {
			print_error(vformat("Cannot get property '%s' node not found.", p_name));
			return false;
		}
		r_ret = property_node.first->get_parameter(property_node.second);
		return true;
	}

	return false;
}

void BLTAnimationGraph::_get_property_list(List<PropertyInfo> *p_list) const {
	if (properties_dirty) {
		_update_properties();
	}

	for (const PropertyInfo &E : properties) {
		p_list->push_back(E);
	}
}

void BLTAnimationGraph::_graph_changed(const StringName &node_name) {
	if (properties_dirty) {
		return;
	}

	callable_mp(this, &BLTAnimationGraph::_update_properties).call_deferred();
	callable_mp(this, &BLTAnimationGraph::_setup_graph).call_deferred();

	properties_dirty = true;
}

void BLTAnimationGraph::_notification(int p_what) {
	GodotProfileZone("BLTAnimationGraph::_notification");

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
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

void BLTAnimationGraph::set_active(bool p_active) {
	if (active == p_active) {
		return;
	}

	active = p_active;
	_set_process(processing, true);
}

bool BLTAnimationGraph::is_active() const {
	return active;
}

void BLTAnimationGraph::set_callback_mode_process(AnimationMixer::AnimationCallbackModeProcess p_mode) {
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

AnimationMixer::AnimationCallbackModeProcess BLTAnimationGraph::get_callback_mode_process() const {
	return callback_mode_process;
}

void BLTAnimationGraph::set_callback_mode_method(AnimationMixer::AnimationCallbackModeMethod p_mode) {
	callback_mode_method = p_mode;
	emit_signal(SNAME("mixer_updated"));
}

AnimationMixer::AnimationCallbackModeMethod BLTAnimationGraph::get_callback_mode_method() const {
	return callback_mode_method;
}

void BLTAnimationGraph::set_callback_mode_discrete(AnimationMixer::AnimationCallbackModeDiscrete p_mode) {
	callback_mode_discrete = p_mode;
	emit_signal(SNAME("mixer_updated"));
}

AnimationMixer::AnimationCallbackModeDiscrete BLTAnimationGraph::get_callback_mode_discrete() const {
	return callback_mode_discrete;
}

void BLTAnimationGraph::print_property_list() const {
	_update_properties();
	print_line("animation graph params:");
	for (unsigned int i = 0; i < parameter_to_node_parameter_map.size(); i++) {
		print_line(vformat("%03d: %s", i, parameter_to_node_parameter_map.get_by_index(i).key));
	}
}

void BLTAnimationGraph::set_animation_player(const NodePath &p_path) {
	print_line(vformat("set_animation_player(%s) ", p_path));

	animation_player_path = p_path;

	_setup_evaluation_context();
	_setup_graph();

	emit_signal(SNAME("animation_player_changed")); // Needs to unpin AnimationPlayerEditor.
}

NodePath BLTAnimationGraph::get_animation_player() const {
	return animation_player_path;
}

void BLTAnimationGraph::set_root_animation_node(const Ref<BLTAnimationNode> &p_animation_node) {
	if (p_animation_node.is_null()) {
		print_line("setting root node to node null");
	} else {
		print_line(vformat("setting root node to node %s", p_animation_node->get_name()));
	}

	if (root_animation_node.is_valid()) {
		root_animation_node->disconnect(SNAME("node_changed"), callable_mp(this, &BLTAnimationGraph::_graph_changed));
	}

	root_animation_node = p_animation_node;
	root_animation_node->node_path = root_animation_node->get_name();

	if (root_animation_node.is_valid()) {
		_setup_graph();
		root_animation_node->connect(SNAME("node_changed"), callable_mp(this, &BLTAnimationGraph::_graph_changed));
	}

	properties_dirty = true;

	update_configuration_warnings();
}

Ref<BLTAnimationNode> BLTAnimationGraph::get_root_animation_node() const {
	return root_animation_node;
}

void BLTAnimationGraph::set_skeleton(const NodePath &p_path) {
	print_line(vformat("set_skeleton(%s) ", p_path));

	skeleton_path = p_path;

	_setup_evaluation_context();
	_setup_graph();

	emit_signal(SNAME("skeleton_changed")); // Needs to unpin AnimationPlayerEditor.
}

NodePath BLTAnimationGraph::get_skeleton() const {
	return skeleton_path;
}

void BLTAnimationGraph::_process_graph(double p_delta, bool p_update_only) {
	if (!root_animation_node.is_valid() || is_graph_initialization_valid == false) {
		return;
	}

	if (graph_context.skeleton_3d == nullptr || graph_context.animation_player == nullptr) {
		return;
	}

	GodotProfileZone("BLTAnimationGraph::_process_graph");

	graph_context.graph_process_delta_time = p_delta;

	_update_properties();

	AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
	root_animation_node->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	root_animation_node->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	root_animation_node->update_time(p_delta);
	root_animation_node->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	_apply_animation_data(*graph_output);

	graph_context.animation_data_allocator.free(graph_output);
}

void BLTAnimationGraph::_apply_animation_data(const AnimationData &output_data) const {
	GodotProfileZone("BLTAnimationGraph::_apply_animation_data");

	for (const KeyValue<Animation::TrackCacheID, size_t> &K : output_data.value_buffer_offset) {
		const AnimationData::TrackValue *track_value = output_data.get_value<AnimationData::TrackValue>(K.key);
		switch (track_value->type) {
			case AnimationData::TrackType::TYPE_POSITION_3D:
			case AnimationData::TrackType::TYPE_ROTATION_3D: {
				const AnimationData::TransformTrackValue *transform_track_value = static_cast<const AnimationData::TransformTrackValue *>(track_value);

				if (transform_track_value->bone_idx != -1) {
					if (transform_track_value->loc_used) {
						graph_context.skeleton_3d->set_bone_pose_position(transform_track_value->bone_idx, transform_track_value->loc);
					}

					if (transform_track_value->rot_used) {
						graph_context.skeleton_3d->set_bone_pose_rotation(transform_track_value->bone_idx, transform_track_value->rot);
					}
				} else {
					assert(false && "Not yet implemented!");
				}

				break;
			}
			default: {
				print_line(vformat("Unsupported track type %d", track_value->type));
				break;
			}
		}
	}
}

void BLTAnimationGraph::_set_process(bool p_process, bool p_force) {
	if (processing == p_process && !p_force) {
		return;
	}

	set_physics_process_internal(false);
	set_process_internal(true);

	processing = p_process;
}

void BLTAnimationGraph::_setup_animation_player() {
	if (!is_inside_tree()) {
		return;
	}

	graph_context.animation_player = Object::cast_to<AnimationPlayer>(get_node_or_null(animation_player_path));
	print_line(vformat("AnimationPlayer of graph %x is now %x", (uintptr_t)(this), (uintptr_t)graph_context.animation_player));
}

void BLTAnimationGraph::_setup_evaluation_context() {
	print_line("_setup_evaluation_context()");
	_cleanup_evaluation_context();

	_setup_animation_player();
	graph_context.skeleton_3d = Object::cast_to<Skeleton3D>(get_node_or_null(skeleton_path));
}

void BLTAnimationGraph::_cleanup_evaluation_context() {
	graph_context.animation_player = nullptr;
	graph_context.skeleton_3d = nullptr;
}

void BLTAnimationGraph::_setup_graph() {
	if (graph_context.animation_player == nullptr || graph_context.skeleton_3d == nullptr || !root_animation_node.is_valid()) {
		return;
	}

	print_line(vformat("_setup_graph() on graph %x and root node %x", (uintptr_t)(void *)(this), (uintptr_t)(root_animation_node.ptr())));
	graph_context.validation_messages.clear();
	is_graph_initialization_valid = root_animation_node->initialize(graph_context);
	if (!is_graph_initialization_valid) {
		String animation_graph_path;
		if (Engine::get_singleton()->is_editor_hint()) {
			animation_graph_path = String(get_tree()->get_edited_scene_root()->get_path_to(this));
		} else {
			animation_graph_path = String(get_tree()->get_root()->get_path_to(this));
		}

		print_line(vformat("BLTAnimationGraph %s invalid:", animation_graph_path));

		for (const String &message : graph_context.validation_messages) {
			print_line(message);
		}
	}
}

BLTAnimationGraph::BLTAnimationGraph() {
}
