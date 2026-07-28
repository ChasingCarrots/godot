/**************************************************************************/
/*  physical_bone_simulator_3d.cpp                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "physical_bone_simulator_3d.h"

#include "core/config/engine.h"
#include "core/io/marshalls.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/physics/physical_bone_3d.h"

namespace {

constexpr uint8_t POSE_SNAPSHOT_VERSION = 1;
constexpr int POSE_SNAPSHOT_HEADER_SIZE = 2;
constexpr int POSE_SNAPSHOT_ROOT_SIZE = 16;
constexpr int POSE_SNAPSHOT_BONE_SIZE = 4; 

uint32_t compress_quaternion(const Quaternion &p_quaternion) {
	Quaternion q = p_quaternion.normalized();
	int largest = 0;
	real_t largest_abs = Math::abs(q[0]);
	for (int i = 1; i < 4; i++) {
		const real_t component_abs = Math::abs(q[i]);
		if (component_abs > largest_abs) {
			largest_abs = component_abs;
			largest = i;
		}
	}
	if (q[largest] < 0.0) {
		q = -q;
	}

	uint32_t packed = static_cast<uint32_t>(largest) << 30;
	int shift = 20;
	for (int i = 0; i < 4; i++) {
		if (i == largest) {
			continue;
		}
		const real_t normalized = CLAMP(q[i] * real_t(Math::SQRT2), real_t(-1.0), real_t(1.0));
		const uint32_t quantized = static_cast<uint32_t>(Math::round((normalized * 0.5 + 0.5) * 1023.0));
		packed |= (quantized & 0x3FF) << shift;
		shift -= 10;
	}
	return packed;
}

Quaternion decompress_quaternion(uint32_t p_packed) {
	const int largest = static_cast<int>(p_packed >> 30);
	real_t components[4] = { 0.0, 0.0, 0.0, 0.0 };
	real_t sum_of_squares = 0.0;
	int shift = 20;
	for (int i = 0; i < 4; i++) {
		if (i == largest) {
			continue;
		}
		const uint32_t quantized = (p_packed >> shift) & 0x3FF;
		shift -= 10;
		const real_t component = (real_t(quantized) / 1023.0 * 2.0 - 1.0) / real_t(Math::SQRT2);
		components[i] = component;
		sum_of_squares += component * component;
	}
	components[largest] = Math::sqrt(MAX(real_t(0.0), real_t(1.0) - sum_of_squares));
	return Quaternion(components[0], components[1], components[2], components[3]).normalized();
}

}

void PhysicalBoneSimulator3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	if (p_old) {
		if (p_old->is_connected(SNAME("bone_list_changed"), callable_mp(this, &PhysicalBoneSimulator3D::_bone_list_changed))) {
			p_old->disconnect(SNAME("bone_list_changed"), callable_mp(this, &PhysicalBoneSimulator3D::_bone_list_changed));
		}
		if (p_old->is_connected(SceneStringName(pose_updated), callable_mp(this, &PhysicalBoneSimulator3D::_pose_updated))) {
			p_old->disconnect(SceneStringName(pose_updated), callable_mp(this, &PhysicalBoneSimulator3D::_pose_updated));
		}
	}
	if (p_new) {
		if (!p_new->is_connected(SNAME("bone_list_changed"), callable_mp(this, &PhysicalBoneSimulator3D::_bone_list_changed))) {
			p_new->connect(SNAME("bone_list_changed"), callable_mp(this, &PhysicalBoneSimulator3D::_bone_list_changed));
		}
		if (!p_new->is_connected(SceneStringName(pose_updated), callable_mp(this, &PhysicalBoneSimulator3D::_pose_updated))) {
			p_new->connect(SceneStringName(pose_updated), callable_mp(this, &PhysicalBoneSimulator3D::_pose_updated));
		}
	}
	_bone_list_changed();
}

void PhysicalBoneSimulator3D::_bone_list_changed() {
	bones.clear();
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	for (int i = 0; i < skeleton->get_bone_count(); i++) {
		SimulatedBone sb;
		sb.parent = skeleton->get_bone_parent(i);
		sb.child_bones = skeleton->get_bone_children(i);
		bones.push_back(sb);
	}
	_rebuild_physical_bones_cache();
	_pose_updated();
}

void PhysicalBoneSimulator3D::_pose_updated() {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton || simulating || playback) {
		return;
	}
	// If this triggers that means that we likely haven't rebuilt the bone list yet.
	if (skeleton->get_bone_count() != (int)bones.size()) {
		// NOTE: this is re-entrant and will call _pose_updated again.
		_bone_list_changed();
	} else {
		for (int i = 0; i < skeleton->get_bone_count(); i++) {
			_bone_pose_updated(skeleton, i);
		}
	}
}

void PhysicalBoneSimulator3D::_bone_pose_updated(Skeleton3D *p_skeleton, int p_bone_id) {
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_bone_id, bones.size());
	bones[p_bone_id].global_pose = p_skeleton->get_bone_global_pose(p_bone_id);
}

void PhysicalBoneSimulator3D::_set_active(bool p_active) {
	if (!Engine::get_singleton()->is_editor_hint()) {
		_reset_physical_bones_state();
	}
}

void PhysicalBoneSimulator3D::_reset_physical_bones_state() {
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		if (bones[i].physical_bone) {
			bones[i].physical_bone->reset_physics_simulation_state();
		}
	}
}

bool PhysicalBoneSimulator3D::is_simulating_physics() const {
	return simulating;
}

int PhysicalBoneSimulator3D::find_bone(const String &p_name) const {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return -1;
	}
	return skeleton->find_bone(p_name);
}

String PhysicalBoneSimulator3D::get_bone_name(int p_bone) const {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return String();
	}
	return skeleton->get_bone_name(p_bone);
}

int PhysicalBoneSimulator3D::get_bone_count() const {
	return bones.size();
}

bool PhysicalBoneSimulator3D::is_bone_parent_of(int p_bone, int p_parent_bone_id) const {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return false;
	}
	return skeleton->is_bone_parent_of(p_bone, p_parent_bone_id);
}

void PhysicalBoneSimulator3D::bind_physical_bone_to_bone(int p_bone, PhysicalBone3D *p_physical_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	ERR_FAIL_COND(bones[p_bone].physical_bone);
	ERR_FAIL_NULL(p_physical_bone);
	bones[p_bone].physical_bone = p_physical_bone;

	_rebuild_physical_bones_cache();
}

void PhysicalBoneSimulator3D::unbind_physical_bone_from_bone(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].physical_bone = nullptr;

	_rebuild_physical_bones_cache();
}

PhysicalBone3D *PhysicalBoneSimulator3D::get_physical_bone(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	return bones[p_bone].physical_bone;
}

PhysicalBone3D *PhysicalBoneSimulator3D::get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	if (bones[p_bone].cache_parent_physical_bone) {
		return bones[p_bone].cache_parent_physical_bone;
	}

	return _get_physical_bone_parent(p_bone);
}

PhysicalBone3D *PhysicalBoneSimulator3D::_get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	const int parent_bone = bones[p_bone].parent;
	if (parent_bone < 0) {
		return nullptr;
	}

	PhysicalBone3D *pb = bones[parent_bone].physical_bone;
	if (pb) {
		return pb;
	} else {
		return get_physical_bone_parent(parent_bone);
	}
}

void PhysicalBoneSimulator3D::_rebuild_physical_bones_cache() {
	const int b_size = bones.size();
	for (int i = 0; i < b_size; ++i) {
		PhysicalBone3D *parent_pb = _get_physical_bone_parent(i);
		if (parent_pb != bones[i].cache_parent_physical_bone) {
			bones[i].cache_parent_physical_bone = parent_pb;
			if (bones[i].physical_bone) {
				bones[i].physical_bone->_on_bone_parent_changed();
			}
		}
	}
	_rebuild_simulated_bone_order();
}

void PhysicalBoneSimulator3D::_rebuild_simulated_bone_order() {
	simulated_bone_order.clear();
	const int b_size = bones.size();

	LocalVector<int> depths;
	int max_depth = 0;
	for (int i = 0; i < b_size; ++i) {
		if (!bones[i].physical_bone) {
			continue;
		}
		int depth = 0;
		int walk = bones[i].parent;
		for (int guard = 0; walk >= 0 && walk < b_size && guard < b_size; ++guard) {
			if (bones[walk].physical_bone) {
				depth++;
			}
			walk = bones[walk].parent;
		}
		simulated_bone_order.push_back(i);
		depths.push_back(depth);
		max_depth = MAX(max_depth, depth);
	}

	LocalVector<int> sorted;
	sorted.reserve(simulated_bone_order.size());
	for (int depth = 0; depth <= max_depth; ++depth) {
		for (uint32_t i = 0; i < simulated_bone_order.size(); ++i) {
			if (depths[i] == depth) {
				sorted.push_back(simulated_bone_order[i]);
			}
		}
	}
	simulated_bone_order = sorted;
}

#ifndef DISABLE_DEPRECATED
void _pb_stop_simulation_compat(Node *p_node) {
	PhysicalBoneSimulator3D *ps = Object::cast_to<PhysicalBoneSimulator3D>(p_node);
	if (ps) {
		return; // Prevent conflict.
	}
	for (int i = p_node->get_child_count() - 1; i >= 0; --i) {
		_pb_stop_simulation_compat(p_node->get_child(i));
	}
	PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node);
	if (pb) {
		pb->set_simulate_physics(false);
	}
}
#endif // _DISABLE_DEPRECATED

void _pb_stop_simulation(Node *p_node) {
	for (int i = p_node->get_child_count() - 1; i >= 0; --i) {
		PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node->get_child(i));
		if (!pb) {
			continue;
		}
		_pb_stop_simulation(pb);
	}
	PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node);
	if (pb) {
		pb->set_simulate_physics(false);
	}
}

void PhysicalBoneSimulator3D::physical_bones_stop_simulation() {
	simulating = false;
	_reset_physical_bones_state();
#ifndef DISABLE_DEPRECATED
	if (is_compat) {
		Skeleton3D *sk = get_skeleton();
		if (sk) {
			_pb_stop_simulation_compat(sk);
		}
	} else {
		_pb_stop_simulation(this);
	}
#else
	_pb_stop_simulation(this);
#endif // _DISABLE_DEPRECATED
}

#ifndef DISABLE_DEPRECATED
void _pb_start_simulation_compat(const PhysicalBoneSimulator3D *p_simulator, Node *p_node, const Vector<int> &p_sim_bones) {
	PhysicalBoneSimulator3D *ps = Object::cast_to<PhysicalBoneSimulator3D>(p_node);
	if (ps) {
		return; // Prevent conflict.
	}
	for (int i = p_node->get_child_count() - 1; i >= 0; --i) {
		_pb_start_simulation_compat(p_simulator, p_node->get_child(i), p_sim_bones);
	}
	PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node);
	if (pb) {
		if (p_sim_bones.is_empty()) { // If no bones are specified, activate ragdoll on full body.
			pb->set_simulate_physics(true);
		} else {
			for (int i = p_sim_bones.size() - 1; i >= 0; --i) {
				if (p_sim_bones[i] == pb->get_bone_id() || p_simulator->is_bone_parent_of(pb->get_bone_id(), p_sim_bones[i])) {
					pb->set_simulate_physics(true);
					break;
				}
			}
		}
	}
}
#endif // _DISABLE_DEPRECATED

void _pb_start_simulation(const PhysicalBoneSimulator3D *p_simulator, Node *p_node, const Vector<int> &p_sim_bones) {
	for (int i = p_node->get_child_count() - 1; i >= 0; --i) {
		PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node->get_child(i));
		if (!pb) {
			continue;
		}
		_pb_start_simulation(p_simulator, pb, p_sim_bones);
	}
	PhysicalBone3D *pb = Object::cast_to<PhysicalBone3D>(p_node);
	if (pb) {
		if (p_sim_bones.is_empty()) { // If no bones are specified, activate ragdoll on full body.
			pb->set_simulate_physics(true);
		} else {
			for (int i = p_sim_bones.size() - 1; i >= 0; --i) {
				if (p_sim_bones[i] == pb->get_bone_id() || p_simulator->is_bone_parent_of(pb->get_bone_id(), p_sim_bones[i])) {
					pb->set_simulate_physics(true);
					break;
				}
			}
		}
	}
}

void PhysicalBoneSimulator3D::physical_bones_start_simulation_on(const TypedArray<StringName> &p_bones) {
	_pose_updated();

	simulating = true;
	_reset_physical_bones_state();

	Vector<int> sim_bones;
	if (p_bones.size() > 0) {
		sim_bones.resize(p_bones.size());
		int c = 0;
		for (int i = sim_bones.size() - 1; i >= 0; --i) {
			int bone_id = find_bone(p_bones[i]);
			if (bone_id != -1) {
				sim_bones.write[c++] = bone_id;
			}
		}
		sim_bones.resize(c);
	}

#ifndef DISABLE_DEPRECATED
	if (is_compat) {
		Skeleton3D *sk = get_skeleton();
		if (sk) {
			_pb_start_simulation_compat(this, sk, sim_bones);
		}
	} else {
		_pb_start_simulation(this, this, sim_bones);
	}
#else
	_pb_start_simulation(this, this, sim_bones);
#endif // _DISABLE_DEPRECATED
}

void _physical_bones_add_remove_collision_exception(bool p_add, Node *p_node, RID p_exception) {
	for (int i = p_node->get_child_count() - 1; i >= 0; --i) {
		_physical_bones_add_remove_collision_exception(p_add, p_node->get_child(i), p_exception);
	}

	CollisionObject3D *co = Object::cast_to<CollisionObject3D>(p_node);
	if (co) {
		if (p_add) {
			PhysicsServer3D::get_singleton()->body_add_collision_exception(co->get_rid(), p_exception);
		} else {
			PhysicsServer3D::get_singleton()->body_remove_collision_exception(co->get_rid(), p_exception);
		}
	}
}

void PhysicalBoneSimulator3D::physical_bones_add_collision_exception(RID p_exception) {
	_physical_bones_add_remove_collision_exception(true, this, p_exception);
}

void PhysicalBoneSimulator3D::physical_bones_remove_collision_exception(RID p_exception) {
	_physical_bones_add_remove_collision_exception(false, this, p_exception);
}

Transform3D PhysicalBoneSimulator3D::get_bone_global_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose;
}

void PhysicalBoneSimulator3D::set_bone_global_pose(int p_bone, const Transform3D &p_pose) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose = p_pose;
}

void PhysicalBoneSimulator3D::set_follow_animation(bool p_enable) {
	follow_animation = p_enable;
}

bool PhysicalBoneSimulator3D::is_following_animation() const {
	return follow_animation;
}

void PhysicalBoneSimulator3D::set_muscle_stiffness(real_t p_stiffness) {
	muscle_stiffness = MAX(0.0, p_stiffness);
}

real_t PhysicalBoneSimulator3D::get_muscle_stiffness() const {
	return muscle_stiffness;
}

void PhysicalBoneSimulator3D::set_muscle_damping(real_t p_damping) {
	muscle_damping = CLAMP(p_damping, 0.0, 1.0);
}

real_t PhysicalBoneSimulator3D::get_muscle_damping() const {
	return muscle_damping;
}

void PhysicalBoneSimulator3D::set_muscle_max_speed(real_t p_speed) {
	muscle_max_speed = MAX(0.0, p_speed);
}

real_t PhysicalBoneSimulator3D::get_muscle_max_speed() const {
	return muscle_max_speed;
}

void PhysicalBoneSimulator3D::set_max_stretch_ratio(real_t p_ratio) {
	max_stretch_ratio = MAX(0.0, p_ratio);
}

real_t PhysicalBoneSimulator3D::get_max_stretch_ratio() const {
	return max_stretch_ratio;
}

void PhysicalBoneSimulator3D::set_max_body_linear_speed(real_t p_speed) {
	max_body_linear_speed = MAX(0.0, p_speed);
}

real_t PhysicalBoneSimulator3D::get_max_body_linear_speed() const {
	return max_body_linear_speed;
}

void PhysicalBoneSimulator3D::set_max_body_angular_speed(real_t p_speed) {
	max_body_angular_speed = MAX(0.0, p_speed);
}

real_t PhysicalBoneSimulator3D::get_max_body_angular_speed() const {
	return max_body_angular_speed;
}

void PhysicalBoneSimulator3D::set_body_linear_velocity(const Vector3 &p_velocity) {
	body_linear_velocity = p_velocity;
}

Vector3 PhysicalBoneSimulator3D::get_body_linear_velocity() const {
	return body_linear_velocity;
}

void PhysicalBoneSimulator3D::set_simulation_space(const Transform3D &p_transform) {
	simulation_space = p_transform;
	simulation_space_inverse = p_transform.affine_inverse();
}

void PhysicalBoneSimulator3D::relocalize_simulation(const Transform3D &p_delta) {
	for (uint32_t i = 0; i < bones.size(); i++) {
		if (bones[i].physical_bone && bones[i].physical_bone->is_simulating_physics()) {
			bones[i].physical_bone->relocalize(p_delta);
		}
	}
}

Transform3D PhysicalBoneSimulator3D::get_simulation_space() const {
	return simulation_space;
}

Transform3D PhysicalBoneSimulator3D::get_simulation_space_inverse() const {
	return simulation_space_inverse;
}

void PhysicalBoneSimulator3D::set_playback(bool p_enable) {
	playback = p_enable;
}

bool PhysicalBoneSimulator3D::is_playback() const {
	return playback;
}

PackedByteArray PhysicalBoneSimulator3D::get_pose_snapshot() {
	PackedByteArray snapshot;
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton || simulated_bone_order.is_empty()) {
		return snapshot;
	}
	ERR_FAIL_COND_V(skeleton->get_bone_count() != (int)bones.size(), snapshot);

	const int bone_count = simulated_bone_order.size();
	ERR_FAIL_COND_V_MSG(bone_count > 255, snapshot, "Pose snapshots support at most 255 simulated bones.");

	snapshot.resize(POSE_SNAPSHOT_HEADER_SIZE + POSE_SNAPSHOT_ROOT_SIZE + (bone_count - 1) * POSE_SNAPSHOT_BONE_SIZE);
	uint8_t *write = snapshot.ptrw();

	write[0] = POSE_SNAPSHOT_VERSION;
	write[1] = static_cast<uint8_t>(bone_count);
	int offset = POSE_SNAPSHOT_HEADER_SIZE;

	for (int i = 0; i < bone_count; i++) {
		const int bone_id = simulated_bone_order[i];
		const Transform3D &pose = bones[bone_id].global_pose;

		if (i == 0) {
			offset += encode_float(pose.origin.x, &write[offset]);
			offset += encode_float(pose.origin.y, &write[offset]);
			offset += encode_float(pose.origin.z, &write[offset]);
			offset += encode_uint32(compress_quaternion(pose.basis.get_rotation_quaternion()), &write[offset]);
			continue;
		}

		PhysicalBone3D *parent_pb = get_physical_bone_parent(bone_id);
		const int parent_bone = (parent_pb && parent_pb->get_bone_id() != -1) ? parent_pb->get_bone_id() : -1;
		if (parent_bone == -1) {
			offset += encode_uint32(compress_quaternion(pose.basis.get_rotation_quaternion()), &write[offset]);
			continue;
		}

		ERR_FAIL_INDEX_V(parent_bone, (int)bones.size(), PackedByteArray());
		const Basis relative = bones[parent_bone].global_pose.basis.inverse() * pose.basis;
		offset += encode_uint32(compress_quaternion(relative.get_rotation_quaternion()), &write[offset]);
	}

	return snapshot;
}

bool PhysicalBoneSimulator3D::_decode_pose_snapshot(const PackedByteArray &p_snapshot, Vector3 &r_root_origin, LocalVector<Quaternion> &r_rotations) const {
	const int bone_count = simulated_bone_order.size();
	const int expected_size = POSE_SNAPSHOT_HEADER_SIZE + POSE_SNAPSHOT_ROOT_SIZE + (bone_count - 1) * POSE_SNAPSHOT_BONE_SIZE;
	ERR_FAIL_COND_V_MSG(p_snapshot.size() != expected_size, false, "Pose snapshot size does not match this simulator's bone set.");

	const uint8_t *read = p_snapshot.ptr();
	ERR_FAIL_COND_V_MSG(read[0] != POSE_SNAPSHOT_VERSION, false, "Pose snapshot version mismatch.");
	ERR_FAIL_COND_V_MSG(read[1] != bone_count, false, "Pose snapshot bone count does not match this simulator.");

	r_rotations.resize(bone_count);
	int offset = POSE_SNAPSHOT_HEADER_SIZE;

	for (int i = 0; i < bone_count; i++) {
		if (i == 0) {
			r_root_origin.x = decode_float(&read[offset]);
			offset += 4;
			r_root_origin.y = decode_float(&read[offset]);
			offset += 4;
			r_root_origin.z = decode_float(&read[offset]);
			offset += 4;
		}
		r_rotations[i] = decompress_quaternion(decode_uint32(&read[offset]));
		offset += 4;
	}
	return true;
}

void PhysicalBoneSimulator3D::_rebuild_poses_from_locals(const Vector3 &p_root_origin, const LocalVector<Quaternion> &p_rotations) {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	const int bone_count = simulated_bone_order.size();

	for (int i = 0; i < bone_count; i++) {
		const int bone_id = simulated_bone_order[i];

		if (i == 0) {
			bones[bone_id].global_pose = Transform3D(Basis(p_rotations[i]), p_root_origin);
			continue;
		}

		PhysicalBone3D *parent_pb = get_physical_bone_parent(bone_id);
		const int parent_bone = (parent_pb && parent_pb->get_bone_id() != -1) ? parent_pb->get_bone_id() : -1;
		if (parent_bone == -1) {
			bones[bone_id].global_pose = Transform3D(Basis(p_rotations[i]), bones[bone_id].global_pose.origin);
			continue;
		}
		ERR_FAIL_INDEX(parent_bone, (int)bones.size());

		const Transform3D parent_rest = skeleton->get_bone_global_rest(parent_bone);
		const Transform3D self_rest = skeleton->get_bone_global_rest(bone_id);
		const Vector3 rest_offset = parent_rest.basis.inverse().xform(self_rest.origin - parent_rest.origin);

		const Transform3D parent_pose = bones[parent_bone].global_pose;
		bones[bone_id].global_pose = Transform3D(
				parent_pose.basis * Basis(p_rotations[i]),
				parent_pose.origin + parent_pose.basis.xform(rest_offset));
	}
}

void PhysicalBoneSimulator3D::apply_pose_snapshot(const PackedByteArray &p_snapshot) {
	if (!get_skeleton() || simulated_bone_order.is_empty()) {
		return;
	}
	ERR_FAIL_COND(get_skeleton()->get_bone_count() != (int)bones.size());

	Vector3 root_origin;
	LocalVector<Quaternion> rotations;
	if (!_decode_pose_snapshot(p_snapshot, root_origin, rotations)) {
		return;
	}
	_rebuild_poses_from_locals(root_origin, rotations);
}

void PhysicalBoneSimulator3D::apply_pose_snapshot_interpolated(const PackedByteArray &p_from, const PackedByteArray &p_to, real_t p_weight) {
	if (!get_skeleton() || simulated_bone_order.is_empty()) {
		return;
	}
	ERR_FAIL_COND(get_skeleton()->get_bone_count() != (int)bones.size());

	Vector3 from_origin;
	Vector3 to_origin;
	LocalVector<Quaternion> from_rotations;
	LocalVector<Quaternion> to_rotations;
	if (!_decode_pose_snapshot(p_from, from_origin, from_rotations)) {
		return;
	}
	if (!_decode_pose_snapshot(p_to, to_origin, to_rotations)) {
		return;
	}

	const real_t weight = CLAMP(p_weight, real_t(0.0), real_t(1.0));
	LocalVector<Quaternion> blended;
	blended.resize(from_rotations.size());
	for (uint32_t i = 0; i < from_rotations.size(); i++) {
		blended[i] = from_rotations[i].slerp(to_rotations[i], weight);
	}
	_rebuild_poses_from_locals(from_origin.lerp(to_origin, weight), blended);
}

Transform3D PhysicalBoneSimulator3D::get_bone_animation_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].animation_global_pose;
}

void PhysicalBoneSimulator3D::_process_modification(double p_delta) {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	ERR_FAIL_COND(skeleton->get_bone_count() != (int)bones.size());

	if (playback) {
		for (const int bone_id : simulated_bone_order) {
			skeleton->set_bone_global_pose(bone_id, bones[bone_id].global_pose);
			bones[bone_id].physical_bone->reset_to_rest_position();
		}
		return;
	}

	if (simulating && follow_animation) {
		for (int i = 0; i < skeleton->get_bone_count(); i++) {
			if (bones[i].physical_bone && bones[i].physical_bone->is_simulating_physics()) {
				bones[i].animation_global_pose = skeleton->get_bone_global_pose(i);
			}
		}
	}

	for (int i = 0; i < skeleton->get_bone_count(); i++) {
		if (!bones[i].physical_bone) {
			continue;
		}
		if (bones[i].physical_bone->is_simulating_physics() == false) {
			_bone_pose_updated(skeleton, i);
			bones[i].physical_bone->reset_to_rest_position();
			bones[i].physical_bone->track_kinematic_velocity(p_delta);
		} else if (simulating) {
			skeleton->set_bone_global_pose(i, bones[i].global_pose);
		}
	}
}

void PhysicalBoneSimulator3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_simulating_physics"), &PhysicalBoneSimulator3D::is_simulating_physics);

	ClassDB::bind_method(D_METHOD("set_follow_animation", "enable"), &PhysicalBoneSimulator3D::set_follow_animation);
	ClassDB::bind_method(D_METHOD("is_following_animation"), &PhysicalBoneSimulator3D::is_following_animation);
	ClassDB::bind_method(D_METHOD("set_muscle_stiffness", "stiffness"), &PhysicalBoneSimulator3D::set_muscle_stiffness);
	ClassDB::bind_method(D_METHOD("get_muscle_stiffness"), &PhysicalBoneSimulator3D::get_muscle_stiffness);
	ClassDB::bind_method(D_METHOD("set_muscle_damping", "damping"), &PhysicalBoneSimulator3D::set_muscle_damping);
	ClassDB::bind_method(D_METHOD("get_muscle_damping"), &PhysicalBoneSimulator3D::get_muscle_damping);
	ClassDB::bind_method(D_METHOD("set_muscle_max_speed", "speed"), &PhysicalBoneSimulator3D::set_muscle_max_speed);
	ClassDB::bind_method(D_METHOD("get_muscle_max_speed"), &PhysicalBoneSimulator3D::get_muscle_max_speed);
	ClassDB::bind_method(D_METHOD("set_max_stretch_ratio", "ratio"), &PhysicalBoneSimulator3D::set_max_stretch_ratio);
	ClassDB::bind_method(D_METHOD("get_max_stretch_ratio"), &PhysicalBoneSimulator3D::get_max_stretch_ratio);
	ClassDB::bind_method(D_METHOD("set_max_body_linear_speed", "speed"), &PhysicalBoneSimulator3D::set_max_body_linear_speed);
	ClassDB::bind_method(D_METHOD("get_max_body_linear_speed"), &PhysicalBoneSimulator3D::get_max_body_linear_speed);
	ClassDB::bind_method(D_METHOD("set_max_body_angular_speed", "speed"), &PhysicalBoneSimulator3D::set_max_body_angular_speed);
	ClassDB::bind_method(D_METHOD("get_max_body_angular_speed"), &PhysicalBoneSimulator3D::get_max_body_angular_speed);
	ClassDB::bind_method(D_METHOD("set_body_linear_velocity", "velocity"), &PhysicalBoneSimulator3D::set_body_linear_velocity);
	ClassDB::bind_method(D_METHOD("get_body_linear_velocity"), &PhysicalBoneSimulator3D::get_body_linear_velocity);
	ClassDB::bind_method(D_METHOD("set_simulation_space", "transform"), &PhysicalBoneSimulator3D::set_simulation_space);
	ClassDB::bind_method(D_METHOD("get_simulation_space"), &PhysicalBoneSimulator3D::get_simulation_space);
	ClassDB::bind_method(D_METHOD("relocalize_simulation", "delta"), &PhysicalBoneSimulator3D::relocalize_simulation);
	ClassDB::bind_method(D_METHOD("set_playback", "enable"), &PhysicalBoneSimulator3D::set_playback);
	ClassDB::bind_method(D_METHOD("is_playback"), &PhysicalBoneSimulator3D::is_playback);
	ClassDB::bind_method(D_METHOD("get_pose_snapshot"), &PhysicalBoneSimulator3D::get_pose_snapshot);
	ClassDB::bind_method(D_METHOD("apply_pose_snapshot", "snapshot"), &PhysicalBoneSimulator3D::apply_pose_snapshot);
	ClassDB::bind_method(D_METHOD("apply_pose_snapshot_interpolated", "from", "to", "weight"), &PhysicalBoneSimulator3D::apply_pose_snapshot_interpolated);
	ClassDB::bind_method(D_METHOD("get_bone_animation_pose", "bone_idx"), &PhysicalBoneSimulator3D::get_bone_animation_pose);
	ClassDB::bind_method(D_METHOD("get_bone_global_pose", "bone_idx"), &PhysicalBoneSimulator3D::get_bone_global_pose);

	ADD_GROUP("Muscle", "muscle_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "follow_animation"), "set_follow_animation", "is_following_animation");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "muscle_stiffness", PROPERTY_HINT_RANGE, "0,100,0.1,or_greater"), "set_muscle_stiffness", "get_muscle_stiffness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "muscle_damping", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_muscle_damping", "get_muscle_damping");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "muscle_max_speed", PROPERTY_HINT_RANGE, "0,60,0.1,or_greater"), "set_muscle_max_speed", "get_muscle_max_speed");

	ADD_GROUP("Stability", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_stretch_ratio", PROPERTY_HINT_RANGE, "1,4,0.05,or_greater"), "set_max_stretch_ratio", "get_max_stretch_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_body_linear_speed", PROPERTY_HINT_RANGE, "0,50,0.1,or_greater,suffix:m/s"), "set_max_body_linear_speed", "get_max_body_linear_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_body_angular_speed", PROPERTY_HINT_RANGE, "0,60,0.1,or_greater,radians_as_degrees,suffix:°/s"), "set_max_body_angular_speed", "get_max_body_angular_speed");

	ClassDB::bind_method(D_METHOD("physical_bones_stop_simulation"), &PhysicalBoneSimulator3D::physical_bones_stop_simulation);
	ClassDB::bind_method(D_METHOD("physical_bones_start_simulation", "bones"), &PhysicalBoneSimulator3D::physical_bones_start_simulation_on, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("physical_bones_add_collision_exception", "exception"), &PhysicalBoneSimulator3D::physical_bones_add_collision_exception);
	ClassDB::bind_method(D_METHOD("physical_bones_remove_collision_exception", "exception"), &PhysicalBoneSimulator3D::physical_bones_remove_collision_exception);
}

PhysicalBoneSimulator3D::PhysicalBoneSimulator3D() {
}
