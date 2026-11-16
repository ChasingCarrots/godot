#ifndef GEOMETRYHELPER_H
#define GEOMETRYHELPER_H

#include "core/math/vector3.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"

class GroundSurfaceEstimate : public RefCounted {
	GDCLASS(GroundSurfaceEstimate, RefCounted)

	bool hit = false;
	Vector3 normal = Vector3(0.0f, 1.0f, 0.0f);
	float height = 0.0f;
	bool is_plane = false;
	int surface_type = 0;
	int hit_count = 0;
	int inlier_count = 0;
	float consensus_ratio = 0.0f;
	float normal_agreement = 0.0f;

protected:
	static void _bind_methods();

public:
	bool get_hit() const { return hit; }
	void set_hit(bool p_v) { hit = p_v; }

	Vector3 get_normal() const { return normal; }
	void set_normal(const Vector3 &p_v) { normal = p_v; }

	float get_height() const { return height; }
	void set_height(float p_v) { height = p_v; }

	bool get_is_plane() const { return is_plane; }
	void set_is_plane(bool p_v) { is_plane = p_v; }

	int get_surface_type() const { return surface_type; }
	void set_surface_type(int p_v) { surface_type = p_v; }

	int get_hit_count() const { return hit_count; }
	void set_hit_count(int p_v) { hit_count = p_v; }

	int get_inlier_count() const { return inlier_count; }
	void set_inlier_count(int p_v) { inlier_count = p_v; }

	float get_consensus_ratio() const { return consensus_ratio; }
	void set_consensus_ratio(float p_v) { consensus_ratio = p_v; }

	float get_normal_agreement() const { return normal_agreement; }
	void set_normal_agreement(float p_v) { normal_agreement = p_v; }
};

class GeometryHelper : public RefCounted {
	GDCLASS(GeometryHelper, RefCounted)

protected:
	static void _bind_methods();

public:
	static void estimate_ground_surface(
			const PackedVector3Array &p_hits,
			const PackedVector3Array &p_normals,
			const Vector3 &p_sample_pos,
			float p_closest_hit_y,
			bool p_has_closest,
			float p_inlier_dist,
			int p_min_plane_inliers,
			int p_min_cluster_support,
			float p_ratio_threshold,
			float p_min_slope_angle_rad,
			float p_normal_consensus_confidence,
			float p_normal_agreement_cos,
			bool p_was_plane,
			const Ref<GroundSurfaceEstimate> &p_out);
};

#endif // GEOMETRYHELPER_H
