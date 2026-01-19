//
// Created by martin on 03.12.25.
//

#include "synced_animation_node.h"

void SyncedAnimationNode::_bind_methods() {
	ADD_SIGNAL(MethodInfo("tree_changed"));
	ADD_SIGNAL(MethodInfo("animation_node_renamed", PropertyInfo(Variant::INT, "object_id"), PropertyInfo(Variant::STRING, "old_name"), PropertyInfo(Variant::STRING, "new_name")));
	ADD_SIGNAL(MethodInfo("animation_node_removed", PropertyInfo(Variant::INT, "object_id"), PropertyInfo(Variant::STRING, "name")));
}

void SyncedAnimationNode::get_parameter_list(List<PropertyInfo> *r_list) const {
}

Variant SyncedAnimationNode::get_parameter_default_value(const StringName &p_parameter) const {
	return Variant();
}

bool SyncedAnimationNode::is_parameter_read_only(const StringName &p_parameter) const {
	return false;
}

void SyncedAnimationNode::set_parameter(const StringName &p_name, const Variant &p_value) {
}

Variant SyncedAnimationNode::get_parameter(const StringName &p_name) const {
	return Variant();
}

void SyncedAnimationNode::_tree_changed() {
	emit_signal(SNAME("tree_changed"));
}

void SyncedAnimationNode::_animation_node_renamed(const ObjectID &p_oid, const String &p_old_name, const String &p_new_name) {
	emit_signal(SNAME("animation_node_renamed"), p_oid, p_old_name, p_new_name);
}

void SyncedAnimationNode::_animation_node_removed(const ObjectID &p_oid, const StringName &p_node) {
	emit_signal(SNAME("animation_node_removed"), p_oid, p_node);
}

void SyncedBlendTree::_get_property_list(List<PropertyInfo> *p_list) const {
	for (const Ref<SyncedAnimationNode> &node : tree_graph.nodes) {
		String prop_name = node->name;
		if (prop_name != "Output") {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "nodes/" + prop_name + "/node", PROPERTY_HINT_RESOURCE_TYPE, "AnimationNode", PROPERTY_USAGE_NO_EDITOR));
		}
		p_list->push_back(PropertyInfo(Variant::VECTOR2, "nodes/" + prop_name + "/position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
	}

	p_list->push_back(PropertyInfo(Variant::ARRAY, "node_connections", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
}

bool SyncedBlendTree::_get(const StringName &p_name, Variant &r_value) const {
	String prop_name = p_name;
	if (prop_name.begins_with("nodes/")) {
		String node_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);
		int node_index = find_node_index_by_name(node_name);

		if (what == "node") {
			if (node_index != -1) {
				r_value = tree_graph.nodes[node_index];
				return true;
			}
		}

		if (what == "position") {
			if (node_index != -1) {
				r_value = tree_graph.nodes[node_index]->position;
				return true;
			}
		}
	} else if (prop_name == "node_connections") {
		Array conns;
		conns.resize(tree_graph.connections.size() * 3);

		int idx = 0;
		for (const BlendTreeConnection &connection : tree_graph.connections) {
			conns[idx * 3 + 0] = connection.target_node->name;
			conns[idx * 3 + 1] = connection.target_node->get_input_index(connection.target_port_name);
			conns[idx * 3 + 2] = connection.source_node->name;
			idx++;
		}

		r_value = conns;
		return true;
	}

	return false;
}

bool SyncedBlendTree::_set(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name.begins_with("nodes/")) {
		String node_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);

		if (what == "node") {
			Ref<SyncedAnimationNode> anode = p_value;
			if (anode.is_valid()) {
				anode->name = node_name;
				add_node(anode);
			}
			return true;
		}

		if (what == "position") {
			int node_index = find_node_index_by_name(node_name);
			if (node_index > -1) {
				tree_graph.nodes[node_index]->position = p_value;
			}
			return true;
		}
	} else if (prop_name == "node_connections") {
		Array conns = p_value;
		ERR_FAIL_COND_V(conns.size() % 3 != 0, false);

		for (int i = 0; i < conns.size(); i += 3) {
			int target_node_index = find_node_index_by_name(conns[i]);
			int target_node_port_index = conns[i + 1];
			int source_node_index = find_node_index_by_name(conns[i + 2]);

			Ref<SyncedAnimationNode> target_node = tree_graph.nodes[target_node_index];
			Vector<StringName> target_input_names;
			target_node->get_input_names(target_input_names);

			add_connection(tree_graph.nodes[source_node_index], target_node, target_input_names[target_node_port_index]);
		}
		return true;
	}

	return false;
}

void AnimationData::sample_from_animation(const Ref<Animation> &animation, const Skeleton3D *skeleton_3d, double p_time) {
	const LocalVector<Animation::Track *> tracks = animation->get_tracks();
	Animation::Track *const *tracks_ptr = tracks.ptr();

	int count = tracks.size();
	for (int i = 0; i < count; i++) {
		TrackValue *track_value = nullptr;
		const Animation::Track *animation_track = tracks_ptr[i];
		const NodePath &track_node_path = animation_track->path;
		if (!animation_track->enabled) {
			continue;
		}

		Animation::TrackType ttype = animation_track->type;
		switch (ttype) {
			case Animation::TYPE_POSITION_3D:
			case Animation::TYPE_ROTATION_3D: {
				TransformTrackValue *transform_track_value = nullptr;
				if (track_values.has(animation_track->thash)) {
					transform_track_value = static_cast<TransformTrackValue *>(track_values[animation_track->thash]);
				} else {
					transform_track_value = memnew(AnimationData::TransformTrackValue);
				}
				int bone_idx = -1;

				if (track_node_path.get_subname_count() == 1) {
					bone_idx = skeleton_3d->find_bone(track_node_path.get_subname(0));
					if (bone_idx != -1) {
						transform_track_value->bone_idx = bone_idx;
						switch (ttype) {
							case Animation::TYPE_POSITION_3D: {
								animation->try_position_track_interpolate(i, p_time, &transform_track_value->loc);
								transform_track_value->loc_used = true;
								break;
							}
							case Animation::TYPE_ROTATION_3D: {
								animation->try_rotation_track_interpolate(i, p_time, &transform_track_value->rot);
								transform_track_value->rot_used = true;
								break;
							}
							default: {
								assert(false && !"Not yet implemented");
								break;
							}
						}
					}
				} else {
					// TODO
					assert(false && !"Not yet implemented");
				}

				track_value = transform_track_value;
				break;
			}
			default: {
				// TODO
				assert(false && !"Not yet implemented");
				break;
			}
		}

		track_value->track = tracks_ptr[i];
		set_value(animation_track->thash, track_value);
	}
}

bool AnimationSamplerNode::initialize(GraphEvaluationContext &context) {
	SyncedAnimationNode::initialize(context);

	animation = context.animation_player->get_animation(animation_name);
	if (!animation.is_valid()) {
		print_error(vformat("Cannot initialize node %s: animation '%s' not found in animation player.", name, animation_name));
		return false;
	}

	node_time_info.loop_mode = animation->get_loop_mode();

	// Initialize Sync Track from marker
	LocalVector<float> sync_markers;
	int marker_index = 0;
	StringName marker_name = itos(marker_index);
	while (animation->has_marker(marker_name)) {
		sync_markers.push_back(animation->get_marker_time(marker_name));
		marker_index++;
		marker_name = itos(marker_index);
	}

	if (sync_markers.size() > 0) {
		node_time_info.sync_track = SyncTrack::create_from_markers(animation->get_length(), sync_markers);
	} else {
		node_time_info.sync_track = SyncTrack::create_from_markers(animation->get_length(), { 0 });
	}

	return true;
}

void AnimationSamplerNode::update_time(double p_time) {
	SyncedAnimationNode::update_time(p_time);

	if (node_time_info.is_synced) {
		// Any potential looping has already been performed in the sync-controlling node.
		return;
	}

	if (node_time_info.loop_mode != Animation::LOOP_NONE) {
		if (node_time_info.loop_mode == Animation::LOOP_LINEAR) {
			if (!Math::is_zero_approx(animation->get_length())) {
				node_time_info.position = Math::fposmod(node_time_info.position, static_cast<double>(animation->get_length()));
			}
		} else {
			assert(false && !"Ping-pong looping not yet supported");
		}
	}
}

void AnimationSamplerNode::evaluate(GraphEvaluationContext &context, const LocalVector<AnimationData *> &inputs, AnimationData &output) {
	assert(inputs.size() == 0);

	if (node_time_info.is_synced) {
		node_time_info.position = node_time_info.sync_track.calc_ratio_from_sync_time(node_time_info.sync_position) * animation->get_length();
	}

	output.clear();
	output.sample_from_animation(animation, context.skeleton_3d, node_time_info.position);
}

void AnimationSamplerNode::set_animation(const StringName &p_name) {
	animation_name = p_name;
}

StringName AnimationSamplerNode::get_animation() const {
	return animation_name;
}

void AnimationSamplerNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation", "name"), &AnimationSamplerNode::set_animation);
	ClassDB::bind_method(D_METHOD("get_animation"), &AnimationSamplerNode::get_animation);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "animation"), "set_animation", "get_animation");
}

void AnimationBlend2Node::evaluate(GraphEvaluationContext &context, const LocalVector<AnimationData *> &inputs, AnimationData &output) {
	output = *inputs[0];
	output.blend(*inputs[1], blend_weight);
}

void AnimationBlend2Node::set_use_sync(bool p_sync) {
	sync = p_sync;
}

bool AnimationBlend2Node::is_using_sync() const {
	return sync;
}

void AnimationBlend2Node::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_use_sync", "enable"), &AnimationBlend2Node::set_use_sync);
	ClassDB::bind_method(D_METHOD("is_using_sync"), &AnimationBlend2Node::is_using_sync);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync"), "set_use_sync", "is_using_sync");
}

void AnimationBlend2Node::get_parameter_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::FLOAT, blend_weight_pname, PROPERTY_HINT_RANGE, "0,1,0.01,or_less,or_greater"));
}

void AnimationBlend2Node::set_parameter(const StringName &p_name, const Variant &p_value) {
	_set(p_name, p_value);
}

Variant AnimationBlend2Node::get_parameter(const StringName &p_name) const {
	Variant result;
	_get(p_name, result);
	return result;
}

Variant AnimationBlend2Node::get_parameter_default_value(const StringName &p_parameter) const {
	if (p_parameter == blend_weight_pname) {
		return blend_weight;
	}

	return Variant();
}

void AnimationBlend2Node::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::FLOAT, blend_weight_pname, PROPERTY_HINT_RANGE, "0,1,0.01,or_less,or_greater"));
	p_list->push_back(PropertyInfo(Variant::BOOL, sync_pname));
}

bool AnimationBlend2Node::_get(const StringName &p_name, Variant &r_value) const {
	if (p_name == blend_weight_pname) {
		r_value = blend_weight;
		return true;
	}

	if (p_name == sync_pname) {
		r_value = sync;
		return true;
	}

	return false;
}

bool AnimationBlend2Node::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == blend_weight_pname) {
		blend_weight = p_value;
		return true;
	}

	if (p_name == sync_pname) {
		sync = p_value;
		return true;
	}

	return false;
}