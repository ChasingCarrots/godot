#ifndef VOXELGRID_H
#define VOXELGRID_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "core/math/math_funcs.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/math/transform_3d.h"
#include "core/math/aabb.h"
#include "core/variant/variant.h"

// Lightweight voxel occupancy grid for level generation overlap tests.
//
// Occupancy is stored in a dense flat array of occupant counts (one byte per
// voxel) over a fixed, configured bounds. This gives branch-free O(1) access
// with good cache locality, which is what the FrozenBulgur constraint solver
// (backtracking, lots of checks) needs.
//
// Voxels are exchanged across the GDScript boundary as a PackedInt32Array,
// where each voxel is one int32 packing the signed, world-absolute voxel
// coordinate (c = floor(world / voxel_size)) into bit-fields:
//   X: 11 bits (-1024..1023), Y: 10 bits (-512..511), Z: 11 bits (-1024..1023)
// The packing range is independent of grid memory (memory is governed by the
// configured dimensions, not by the packing range).
class VoxelGrid : public RefCounted {
	GDCLASS(VoxelGrid, RefCounted)

	float _voxel_size = 0.0f;
	float _inv_voxel_size = 0.0f;
	Vector3i _origin_voxel; // min-corner voxel coordinate (world-absolute)
	Vector3i _dims; // grid size in voxels (W, H, D)
	LocalVector<uint8_t> _cells; // occupant count per voxel, size W*H*D
	bool _out_of_bounds_occupied = true;
	LocalVector<int> _scratch_idx; // reused by has_overlap_excluding, no per-call alloc

	// --- packed int32 <-> voxel coordinate ---------------------------------
	static _FORCE_INLINE_ int32_t _pack(int32_t x, int32_t y, int32_t z) {
		return (int32_t)(((uint32_t)x & 0x7FFu) |
				(((uint32_t)y & 0x3FFu) << 11) |
				(((uint32_t)z & 0x7FFu) << 21));
	}
	static _FORCE_INLINE_ Vector3i _unpack(int32_t key) {
		// Sign-extend each bit-field by shifting it up to the sign bit and
		// back down with an arithmetic shift.
		const int32_t x = (int32_t)((uint32_t)key << 21) >> 21; // bits 0..10
		const int32_t y = (int32_t)((uint32_t)key << 11) >> 22; // bits 11..20
		const int32_t z = (int32_t)key >> 21; // bits 21..31
		return Vector3i(x, y, z);
	}

	static _FORCE_INLINE_ int32_t _floor_div(float world, float inv_size) {
		return (int32_t)Math::floor(world * inv_size);
	}

	// Returns the linear cell index for a packed voxel, or -1 if out of bounds.
	_FORCE_INLINE_ int _voxel_to_index(int32_t key) const {
		const int32_t lx = ((int32_t)((uint32_t)key << 21) >> 21) - _origin_voxel.x;
		if (lx < 0 || lx >= _dims.x) {
			return -1;
		}
		const int32_t ly = ((int32_t)((uint32_t)key << 11) >> 22) - _origin_voxel.y;
		if (ly < 0 || ly >= _dims.y) {
			return -1;
		}
		const int32_t lz = ((int32_t)key >> 21) - _origin_voxel.z;
		if (lz < 0 || lz >= _dims.z) {
			return -1;
		}
		return lx + ly * _dims.x + lz * _dims.x * _dims.y;
	}

protected:
	static void _bind_methods();

public:
	// Configuration.
	void configure(Vector3 min_corner, Vector3i dimensions, float voxel_size);
	// Convenience: derive the voxel dimensions from a world-space bounds so the
	// grid covers every voxel touched by the AABB (inclusive on both ends).
	void configure_aabb(AABB bounds, float voxel_size);
	bool is_configured() const { return _cells.size() > 0 && _voxel_size > 0.0f; }
	float get_voxel_size() const { return _voxel_size; }
	Vector3i get_dimensions() const { return _dims; }
	void set_out_of_bounds_occupied(bool p_value) { _out_of_bounds_occupied = p_value; }
	bool get_out_of_bounds_occupied() const { return _out_of_bounds_occupied; }

	// Coordinate helpers.
	int world_to_voxel(Vector3 world_pos) const;
	Vector3 voxel_to_world(int voxel) const; // voxel center
	int coords_to_voxel(Vector3i coords) const { return _pack(coords.x, coords.y, coords.z); }
	Vector3i voxel_to_coords(int voxel) const { return _unpack(voxel); }

	// Core operations.
	// Each point's voxel is expanded into a filled cube of (2*grow+1)^3 voxels
	// (grow voxels in every direction). grow_by_voxels=0 yields one voxel per
	// point. Growing closes gaps a sparse point cloud could clip through and
	// enforces a minimum spacing between rooms. Result is de-duplicated.
	PackedInt32Array voxels_from_points(const PackedVector3Array &points, const Transform3D &xform, int grow_by_voxels = 0) const;
	bool has_overlap(const PackedInt32Array &voxels) const;
	// Like has_overlap, but ignores the occupancy contributed by `excluded`
	// (e.g. the room a new room connects to, which is expected to overlap).
	// Subtracts the excluded room's count rather than masking voxel positions,
	// so a real collision with a *third* room on a shared voxel is still caught.
	bool has_overlap_excluding(const PackedInt32Array &voxels, const PackedInt32Array &excluded);
	void mark_occupied(const PackedInt32Array &voxels);
	void mark_unoccupied(const PackedInt32Array &voxels);
	void clear();

	// Debug / inspection.
	int get_occupancy(int voxel) const;
	PackedInt32Array get_occupied_voxels() const;
	int get_occupied_count() const;
};

#endif // VOXELGRID_H
