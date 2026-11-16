#include "GeometryHelper.h"

#include "core/math/math_funcs.h"
#include "core/templates/local_vector.h"

#include <algorithm>

namespace {

enum SurfaceType {
	SURFACE_IN_AIR = 0,
	SURFACE_SPARSE = 1,
	SURFACE_PLANE = 2,
	SURFACE_STEP_UP = 3,
};

static bool solve_3x3(double m00, double m01, double m02,
		double m10, double m11, double m12,
		double m20, double m21, double m22,
		double r0, double r1, double r2,
		double &r_a, double &r_b, double &r_c) {
	const double det = m00 * (m11 * m22 - m12 * m21)
			- m01 * (m10 * m22 - m12 * m20)
			+ m02 * (m10 * m21 - m11 * m20);
	if (Math::abs(det) < 1e-6) {
		return false;
	}
	const double inv = 1.0 / det;
	r_a = (r0 * (m11 * m22 - m12 * m21)
			- m01 * (r1 * m22 - m12 * r2)
			+ m02 * (r1 * m21 - m11 * r2)) * inv;
	r_b = (m00 * (r1 * m22 - m12 * r2)
			- r0 * (m10 * m22 - m12 * m20)
			+ m02 * (m10 * r2 - r1 * m20)) * inv;
	r_c = (m00 * (m11 * r2 - r1 * m21)
			- m01 * (m10 * r2 - r1 * m20)
			+ r0 * (m10 * m21 - m11 * m20)) * inv;
	return true;
}

static bool fit_plane_ls(const Vector3 *p_hits, const int *p_inliers, int p_inlier_count, double &r_a, double &r_b, double &r_c) {
	double sxx = 0.0, sxz = 0.0, sx = 0.0;
	double szz = 0.0, sz = 0.0;
	double sxy = 0.0, szy = 0.0, sy = 0.0;
	for (int i = 0; i < p_inlier_count; i++) {
		const Vector3 &p = p_hits[p_inliers[i]];
		const double px = p.x;
		const double py = p.y;
		const double pz = p.z;
		sxx += px * px;
		sxz += px * pz;
		sx += px;
		szz += pz * pz;
		sz += pz;
		sxy += px * py;
		szy += pz * py;
		sy += py;
	}
	return solve_3x3(
			sxx, sxz, sx,
			sxz, szz, sz,
			sx, sz, (double)p_inlier_count,
			sxy, szy, sy,
			r_a, r_b, r_c);
}

static void consensus_normal_impl(const Vector3 *p_normals, int p_count, float p_cos_thresh,
		bool &r_found, Vector3 &r_normal, float &r_agreement) {
	if (p_count <= 0) {
		r_found = false;
		r_normal = Vector3(0.0f, 1.0f, 0.0f);
		r_agreement = 0.0f;
		return;
	}

	int best_idx = 0;
	int best_count = 0;
	for (int i = 0; i < p_count; i++) {
		int count = 0;
		const Vector3 &ni = p_normals[i];
		for (int j = 0; j < p_count; j++) {
			if (ni.dot(p_normals[j]) >= p_cos_thresh) {
				count++;
			}
		}
		if (count > best_count) {
			best_count = count;
			best_idx = i;
		}
	}

	const Vector3 &seed = p_normals[best_idx];
	Vector3 acc;
	for (int j = 0; j < p_count; j++) {
		if (seed.dot(p_normals[j]) >= p_cos_thresh) {
			acc += p_normals[j];
		}
	}

	if (acc.length_squared() < 1e-8f) {
		r_found = false;
		r_normal = Vector3(0.0f, 1.0f, 0.0f);
		r_agreement = 0.0f;
		return;
	}

	Vector3 consensus = acc.normalized();
	if (consensus.y < 0.0f) {
		consensus = -consensus;
	}
	r_found = true;
	r_normal = consensus;
	r_agreement = (float)best_count / (float)p_count;
}

static bool outliers_form_height_band(const Vector3 *p_hits, const uint8_t *p_is_inlier, int p_count,
		float p_band_height, int p_min_support) {
	LocalVector<float> outlier_ys;
	outlier_ys.reserve(p_count);
	for (int i = 0; i < p_count; i++) {
		if (p_is_inlier[i] == 0) {
			outlier_ys.push_back(p_hits[i].y);
		}
	}
	const int out_n = (int)outlier_ys.size();
	if (out_n < p_min_support) {
		return false;
	}
	for (int i = 0; i < out_n; i++) {
		int count = 0;
		const float yi = outlier_ys[i];
		for (int j = 0; j < out_n; j++) {
			if (Math::abs(outlier_ys[j] - yi) <= p_band_height) {
				count++;
			}
		}
		if (count >= p_min_support) {
			return true;
		}
	}
	return false;
}

static bool highest_supported_height(const Vector3 *p_hits, int p_count, float p_band_height,
		int p_min_support, float &r_mean_y) {
	if (p_count <= 0) {
		return false;
	}
	LocalVector<float> ys;
	ys.resize(p_count);
	for (int i = 0; i < p_count; i++) {
		ys[i] = p_hits[i].y;
	}
	std::sort(ys.ptr(), ys.ptr() + p_count, [](float a, float b) { return a > b; });

	int i = 0;
	while (i < p_count) {
		const float top = ys[i];
		double sum = 0.0;
		int count = 0;
		int j = i;
		while (j < p_count && top - ys[j] <= p_band_height) {
			sum += ys[j];
			count++;
			j++;
		}
		if (count >= p_min_support) {
			r_mean_y = (float)(sum / (double)count);
			return true;
		}
		i = j;
	}
	return false;
}

} // namespace

void GroundSurfaceEstimate::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_hit"), &GroundSurfaceEstimate::get_hit);
	ClassDB::bind_method(D_METHOD("set_hit", "value"), &GroundSurfaceEstimate::set_hit);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hit"), "set_hit", "get_hit");

	ClassDB::bind_method(D_METHOD("get_normal"), &GroundSurfaceEstimate::get_normal);
	ClassDB::bind_method(D_METHOD("set_normal", "value"), &GroundSurfaceEstimate::set_normal);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "normal"), "set_normal", "get_normal");

	ClassDB::bind_method(D_METHOD("get_height"), &GroundSurfaceEstimate::get_height);
	ClassDB::bind_method(D_METHOD("set_height", "value"), &GroundSurfaceEstimate::set_height);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height"), "set_height", "get_height");

	ClassDB::bind_method(D_METHOD("get_is_plane"), &GroundSurfaceEstimate::get_is_plane);
	ClassDB::bind_method(D_METHOD("set_is_plane", "value"), &GroundSurfaceEstimate::set_is_plane);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_plane"), "set_is_plane", "get_is_plane");

	ClassDB::bind_method(D_METHOD("get_surface_type"), &GroundSurfaceEstimate::get_surface_type);
	ClassDB::bind_method(D_METHOD("set_surface_type", "value"), &GroundSurfaceEstimate::set_surface_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "surface_type"), "set_surface_type", "get_surface_type");

	ClassDB::bind_method(D_METHOD("get_hit_count"), &GroundSurfaceEstimate::get_hit_count);
	ClassDB::bind_method(D_METHOD("set_hit_count", "value"), &GroundSurfaceEstimate::set_hit_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hit_count"), "set_hit_count", "get_hit_count");

	ClassDB::bind_method(D_METHOD("get_inlier_count"), &GroundSurfaceEstimate::get_inlier_count);
	ClassDB::bind_method(D_METHOD("set_inlier_count", "value"), &GroundSurfaceEstimate::set_inlier_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "inlier_count"), "set_inlier_count", "get_inlier_count");

	ClassDB::bind_method(D_METHOD("get_consensus_ratio"), &GroundSurfaceEstimate::get_consensus_ratio);
	ClassDB::bind_method(D_METHOD("set_consensus_ratio", "value"), &GroundSurfaceEstimate::set_consensus_ratio);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "consensus_ratio"), "set_consensus_ratio", "get_consensus_ratio");

	ClassDB::bind_method(D_METHOD("get_normal_agreement"), &GroundSurfaceEstimate::get_normal_agreement);
	ClassDB::bind_method(D_METHOD("set_normal_agreement", "value"), &GroundSurfaceEstimate::set_normal_agreement);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_agreement"), "set_normal_agreement", "get_normal_agreement");
}

void GeometryHelper::_bind_methods() {
	ClassDB::bind_static_method("GeometryHelper",
			D_METHOD("estimate_ground_surface",
					"hits", "normals", "sample_pos", "closest_hit_y", "has_closest",
					"inlier_dist", "min_plane_inliers", "min_cluster_support",
					"ratio_threshold", "min_slope_angle_rad",
					"normal_consensus_confidence", "normal_agreement_cos",
					"was_plane", "out_estimate"),
			&GeometryHelper::estimate_ground_surface);
}

void GeometryHelper::estimate_ground_surface(
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
		const Ref<GroundSurfaceEstimate> &p_out) {
	ERR_FAIL_COND(p_out.is_null());

	const int n = p_hits.size();
	const int normal_count = p_normals.size();
	const Vector3 *hits = p_hits.ptr();
	const Vector3 *normals = p_normals.ptr();
	const Vector3 up(0.0f, 1.0f, 0.0f);

	p_out->set_hit(false);
	p_out->set_normal(up);
	p_out->set_height(0.0f);
	p_out->set_is_plane(false);
	p_out->set_surface_type(SURFACE_IN_AIR);
	p_out->set_hit_count(n);
	p_out->set_inlier_count(0);
	p_out->set_consensus_ratio(0.0f);
	p_out->set_normal_agreement(0.0f);

	if (!p_has_closest) {
		return;
	}

	if (n < p_min_plane_inliers) {
		bool cn_found = false;
		Vector3 cn_normal;
		float cn_agreement = 0.0f;
		consensus_normal_impl(normals, normal_count, p_normal_agreement_cos, cn_found, cn_normal, cn_agreement);
		p_out->set_normal_agreement(cn_agreement);

		Vector3 sparse_normal = up;
		if (cn_found && up.angle_to(cn_normal) >= p_min_slope_angle_rad) {
			sparse_normal = cn_normal;
		}
		p_out->set_hit(true);
		p_out->set_normal(sparse_normal);
		p_out->set_height(p_closest_hit_y);
		p_out->set_surface_type(SURFACE_SPARSE);
		return;
	}

	int best_count = 0;
	float best_residual = INFINITY;
	Vector3 best_normal = up;
	float best_offset = 0.0f;

	if (n >= 3) {
		for (int i = 0; i < n; i++) {
			const Vector3 &p0 = hits[i];
			for (int j = i + 1; j < n; j++) {
				const Vector3 e1 = hits[j] - p0;
				for (int k = j + 1; k < n; k++) {
					Vector3 normal = e1.cross(hits[k] - p0);
					if (normal.length_squared() < 1e-8f) {
						continue;
					}
					normal.normalize();
					if (normal.y < 0.0f) {
						normal = -normal;
					}
					const float offset = normal.dot(p0);

					int count = 0;
					float residual = 0.0f;
					for (int m = 0; m < n; m++) {
						const float d = Math::abs(normal.dot(hits[m]) - offset);
						if (d <= p_inlier_dist) {
							count++;
							residual += d;
						}
					}

					if (count > best_count || (count == best_count && residual < best_residual)) {
						best_count = count;
						best_residual = residual;
						best_normal = normal;
						best_offset = offset;
					}
				}
			}
		}
	}

	LocalVector<int> inliers;
	LocalVector<uint8_t> is_inlier;
	is_inlier.resize(n);
	memset(is_inlier.ptr(), 0, n);
	if (best_count > 0) {
		inliers.reserve(best_count);
		for (int m = 0; m < n; m++) {
			if (Math::abs(best_normal.dot(hits[m]) - best_offset) <= p_inlier_dist) {
				inliers.push_back(m);
				is_inlier[m] = 1;
			}
		}
	}

	const float ratio = (float)best_count / (float)n;
	p_out->set_inlier_count(best_count);
	p_out->set_consensus_ratio(ratio);

	bool cn_found = false;
	Vector3 consensus_normal;
	float cn_agreement = 0.0f;
	consensus_normal_impl(normals, normal_count, p_normal_agreement_cos, cn_found, consensus_normal, cn_agreement);
	const bool consensus_ok = cn_found && cn_agreement >= p_normal_consensus_confidence;
	p_out->set_normal_agreement(cn_agreement);

	if (best_count >= p_min_plane_inliers && ratio >= p_ratio_threshold) {
		Vector3 floor_normal;
		float plane_y = 0.0f;
		double a = 0.0, b = 0.0, c = 0.0;
		if (fit_plane_ls(hits, inliers.ptr(), (int)inliers.size(), a, b, c)) {
			floor_normal = Vector3((real_t)-a, (real_t)1.0, (real_t)-b).normalized();
			plane_y = (float)(a * (double)p_sample_pos.x + b * (double)p_sample_pos.z + c);
		} else {
			floor_normal = consensus_ok ? consensus_normal : best_normal;
			plane_y = (best_offset - best_normal.x * p_sample_pos.x - best_normal.z * p_sample_pos.z) / best_normal.y;
		}

		const bool plane_is_slope = up.angle_to(floor_normal) >= p_min_slope_angle_rad;
		const bool normals_say_flat = consensus_ok && up.angle_to(consensus_normal) < p_min_slope_angle_rad;
		const int second_surface_support = p_was_plane ? p_min_cluster_support : MAX(1, p_min_cluster_support - 1);
		const bool has_second_surface = outliers_form_height_band(hits, is_inlier.ptr(), n, p_inlier_dist, second_surface_support);
		if (!((plane_is_slope && normals_say_flat) || has_second_surface)) {
			p_out->set_hit(true);
			p_out->set_normal(floor_normal);
			p_out->set_height(plane_y);
			p_out->set_is_plane(true);
			p_out->set_surface_type(SURFACE_PLANE);
			return;
		}
	}

	float support_y = p_closest_hit_y;
	highest_supported_height(hits, n, p_inlier_dist, p_min_cluster_support, support_y);
	p_out->set_hit(true);
	p_out->set_normal(up);
	p_out->set_height(support_y);
	p_out->set_is_plane(false);
	p_out->set_surface_type(SURFACE_STEP_UP);
}
