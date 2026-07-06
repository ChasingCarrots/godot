/**************************************************************************/
/*  physical_bone_simulator_3d.h                                          */
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

#pragma once

#include "scene/3d/skeleton_modifier_3d.h"

class PhysicalBone3D;

class PhysicalBoneSimulator3D : public SkeletonModifier3D {
	GDCLASS(PhysicalBoneSimulator3D, SkeletonModifier3D);

	bool simulating = false;

	bool follow_animation = false;
	real_t muscle_stiffness = 16.0;
	real_t muscle_damping = 0.4;
	real_t muscle_max_speed = 20.0;

	real_t max_stretch_ratio = 1.5;

	real_t max_body_linear_speed = 0.0; // m/s
	real_t max_body_angular_speed = 0.0; // rad/s

	Vector3 body_linear_velocity;

	Transform3D simulation_space;
	Transform3D simulation_space_inverse;

	struct SimulatedBone {
		int parent;
		Vector<int> child_bones;

		Transform3D global_pose;
		Transform3D animation_global_pose;

		PhysicalBone3D *physical_bone = nullptr;
		PhysicalBone3D *cache_parent_physical_bone = nullptr;

		SimulatedBone() {
			parent = -1;
			global_pose = Transform3D();
			animation_global_pose = Transform3D();
			physical_bone = nullptr;
			cache_parent_physical_bone = nullptr;
		}
	};

	LocalVector<SimulatedBone> bones;

	/// This is a slow API, so it's cached
	PhysicalBone3D *_get_physical_bone_parent(int p_bone);
	void _rebuild_physical_bones_cache();
	void _reset_physical_bones_state();

protected:
	static void _bind_methods();

	virtual void _set_active(bool p_active) override;

	void _bone_list_changed();
	void _pose_updated();
	void _bone_pose_updated(Skeleton3D *skeleton, int p_bone_id);

	virtual void _process_modification(double p_delta) override;

	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;

public:
#ifndef DISABLE_DEPRECATED
	bool is_compat = false;
#endif // _DISABLE_DEPRECATED
	bool is_simulating_physics() const;

	void set_follow_animation(bool p_enable);
	bool is_following_animation() const;
	void set_muscle_stiffness(real_t p_stiffness);
	real_t get_muscle_stiffness() const;
	void set_muscle_damping(real_t p_damping);
	real_t get_muscle_damping() const;
	void set_muscle_max_speed(real_t p_speed);
	real_t get_muscle_max_speed() const;
	void set_max_stretch_ratio(real_t p_ratio);
	real_t get_max_stretch_ratio() const;
	void set_max_body_linear_speed(real_t p_speed);
	real_t get_max_body_linear_speed() const;

	void set_body_linear_velocity(const Vector3 &p_velocity);
	Vector3 get_body_linear_velocity() const;

	void set_simulation_space(const Transform3D &p_transform);
	Transform3D get_simulation_space() const;
	Transform3D get_simulation_space_inverse() const;

	void relocalize_simulation(const Transform3D &p_delta);
	void set_max_body_angular_speed(real_t p_speed);
	real_t get_max_body_angular_speed() const;
	Transform3D get_bone_animation_pose(int p_bone) const;

	int find_bone(const String &p_name) const;
	String get_bone_name(int p_bone) const;
	int get_bone_count() const;
	bool is_bone_parent_of(int p_bone_id, int p_parent_bone_id) const;

	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D &p_pose);

	void bind_physical_bone_to_bone(int p_bone, PhysicalBone3D *p_physical_bone);
	void unbind_physical_bone_from_bone(int p_bone);

	PhysicalBone3D *get_physical_bone(int p_bone);
	PhysicalBone3D *get_physical_bone_parent(int p_bone);

	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<StringName> &p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);

	PhysicalBoneSimulator3D();
};
