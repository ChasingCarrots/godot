#include "VoxelGrid.h"

#include "core/templates/hash_set.h"

#include <string.h>

void VoxelGrid::configure(Vector3 min_corner, Vector3i dimensions, float voxel_size) {
	ERR_FAIL_COND_MSG(voxel_size <= 0.0f, "VoxelGrid: voxel_size must be > 0.");
	ERR_FAIL_COND_MSG(dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0,
			"VoxelGrid: dimensions must be positive on every axis.");

	_voxel_size = voxel_size;
	_inv_voxel_size = 1.0f / voxel_size;
	_dims = dimensions;
	_origin_voxel = Vector3i(
			_floor_div(min_corner.x, _inv_voxel_size),
			_floor_div(min_corner.y, _inv_voxel_size),
			_floor_div(min_corner.z, _inv_voxel_size));

	const int64_t count = (int64_t)_dims.x * (int64_t)_dims.y * (int64_t)_dims.z;
	ERR_FAIL_COND_MSG(count > (int64_t)0x7FFFFFFF, "VoxelGrid: configured volume is too large.");
	_cells.resize((uint32_t)count);
	memset(_cells.ptr(), 0, _cells.size() * sizeof(uint8_t));
}

void VoxelGrid::configure_aabb(AABB bounds, float voxel_size) {
	ERR_FAIL_COND_MSG(voxel_size <= 0.0f, "VoxelGrid: voxel_size must be > 0.");
	const AABB b = bounds.abs(); // tolerate negative-size AABBs
	const float inv = 1.0f / voxel_size;
	const Vector3 max_corner = b.position + b.size;
	// Inclusive voxel span: every voxel the AABB touches on both ends.
	const Vector3i dimensions(
			(int32_t)Math::floor(max_corner.x * inv) - (int32_t)Math::floor(b.position.x * inv) + 1,
			(int32_t)Math::floor(max_corner.y * inv) - (int32_t)Math::floor(b.position.y * inv) + 1,
			(int32_t)Math::floor(max_corner.z * inv) - (int32_t)Math::floor(b.position.z * inv) + 1);
	configure(b.position, dimensions, voxel_size);
}

int VoxelGrid::world_to_voxel(Vector3 world_pos) const {
	const int32_t x = _floor_div(world_pos.x, _inv_voxel_size);
	const int32_t y = _floor_div(world_pos.y, _inv_voxel_size);
	const int32_t z = _floor_div(world_pos.z, _inv_voxel_size);
#ifdef DEV_ENABLED
	DEV_ASSERT(x >= -1024 && x <= 1023 && y >= -512 && y <= 511 && z >= -1024 && z <= 1023);
#endif
	return _pack(x, y, z);
}

Vector3 VoxelGrid::voxel_to_world(int voxel) const {
	const Vector3i c = _unpack(voxel);
	return Vector3(
			((float)c.x + 0.5f) * _voxel_size,
			((float)c.y + 0.5f) * _voxel_size,
			((float)c.z + 0.5f) * _voxel_size);
}

PackedInt32Array VoxelGrid::voxels_from_points(const PackedVector3Array &points, const Transform3D &xform, int grow_by_voxels) const {
	PackedInt32Array result;
	const int n = points.size();
	if (n == 0) {
		return result;
	}
	const int g = grow_by_voxels > 0 ? grow_by_voxels : 0;
	const int side = 2 * g + 1;
	const int cube = side * side * side;
	result.resize(n * cube); // upper bound before de-duplication
	int32_t *out = result.ptrw();
	const Vector3 *p = points.ptr();

	HashSet<int32_t> seen;
	seen.reserve((uint32_t)(n * cube));
	int count = 0;
	for (int i = 0; i < n; i++) {
		const Vector3 w = xform.xform(p[i]);
		const int32_t cx = _floor_div(w.x, _inv_voxel_size);
		const int32_t cy = _floor_div(w.y, _inv_voxel_size);
		const int32_t cz = _floor_div(w.z, _inv_voxel_size);
		for (int dz = -g; dz <= g; dz++) {
			for (int dy = -g; dy <= g; dy++) {
				for (int dx = -g; dx <= g; dx++) {
					const int32_t key = _pack(cx + dx, cy + dy, cz + dz);
					if (!seen.has(key)) {
						seen.insert(key);
						out[count++] = key;
					}
				}
			}
		}
	}
	result.resize(count);
	return result;
}

bool VoxelGrid::has_overlap(const PackedInt32Array &voxels) const {
	const int n = voxels.size();
	const int32_t *v = voxels.ptr();
	const uint8_t *cells = _cells.ptr();
	for (int i = 0; i < n; i++) {
		const int idx = _voxel_to_index(v[i]);
		if (idx < 0) {
			if (_out_of_bounds_occupied) {
				return true;
			}
			continue;
		}
		if (cells[idx] > 0) {
			return true;
		}
	}
	return false;
}

bool VoxelGrid::has_overlap_excluding(const PackedInt32Array &voxels, const PackedInt32Array &excluded) {
	uint8_t *cells = _cells.ptr();

	// Temporarily remove the excluded room's contribution. We only decrement
	// cells that are actually occupied and remember which ones, so the restore
	// pass is exact and there is never an underflow (a not-occupied excluded
	// voxel is simply left at 0).
	const int en = excluded.size();
	const int32_t *ex = excluded.ptr();
	_scratch_idx.resize((uint32_t)en);
	int *idxs = _scratch_idx.ptr();
	for (int i = 0; i < en; i++) {
		const int idx = _voxel_to_index(ex[i]);
		if (idx >= 0 && cells[idx] > 0) {
			cells[idx]--;
			idxs[i] = idx;
		} else {
			idxs[i] = -1;
		}
	}

	bool result = false;
	const int n = voxels.size();
	const int32_t *v = voxels.ptr();
	for (int i = 0; i < n; i++) {
		const int idx = _voxel_to_index(v[i]);
		if (idx < 0) {
			if (_out_of_bounds_occupied) {
				result = true;
				break;
			}
			continue;
		}
		if (cells[idx] > 0) {
			result = true;
			break;
		}
	}

	// Restore exactly what we decremented (must happen even on early-out).
	for (int i = 0; i < en; i++) {
		if (idxs[i] >= 0) {
			cells[idxs[i]]++;
		}
	}
	return result;
}

void VoxelGrid::mark_occupied(const PackedInt32Array &voxels) {
	const int n = voxels.size();
	const int32_t *v = voxels.ptr();
	uint8_t *cells = _cells.ptr();
	for (int i = 0; i < n; i++) {
		const int idx = _voxel_to_index(v[i]);
		if (idx >= 0 && cells[idx] < 255) {
			cells[idx]++;
		}
	}
}

void VoxelGrid::mark_unoccupied(const PackedInt32Array &voxels) {
	const int n = voxels.size();
	const int32_t *v = voxels.ptr();
	uint8_t *cells = _cells.ptr();
	for (int i = 0; i < n; i++) {
		const int idx = _voxel_to_index(v[i]);
		if (idx >= 0 && cells[idx] > 0) {
			cells[idx]--;
		}
	}
}

void VoxelGrid::clear() {
	if (_cells.size() > 0) {
		memset(_cells.ptr(), 0, _cells.size() * sizeof(uint8_t));
	}
}

int VoxelGrid::get_occupancy(int voxel) const {
	const int idx = _voxel_to_index(voxel);
	return idx >= 0 ? (int)_cells[idx] : 0;
}

PackedInt32Array VoxelGrid::get_occupied_voxels() const {
	PackedInt32Array result;
	const int w = _dims.x;
	const int h = _dims.y;
	const int d = _dims.z;
	const uint8_t *cells = _cells.ptr();
	for (int lz = 0; lz < d; lz++) {
		for (int ly = 0; ly < h; ly++) {
			const int row = ly * w + lz * w * h;
			for (int lx = 0; lx < w; lx++) {
				if (cells[lx + row] > 0) {
					result.push_back(_pack(
							lx + _origin_voxel.x,
							ly + _origin_voxel.y,
							lz + _origin_voxel.z));
				}
			}
		}
	}
	return result;
}

int VoxelGrid::get_occupied_count() const {
	int occupied = 0;
	const uint32_t n = _cells.size();
	const uint8_t *cells = _cells.ptr();
	for (uint32_t i = 0; i < n; i++) {
		if (cells[i] > 0) {
			occupied++;
		}
	}
	return occupied;
}

void VoxelGrid::_bind_methods() {
	ClassDB::bind_method(D_METHOD("configure", "min_corner", "dimensions", "voxel_size"), &VoxelGrid::configure);
	ClassDB::bind_method(D_METHOD("configure_aabb", "bounds", "voxel_size"), &VoxelGrid::configure_aabb);
	ClassDB::bind_method(D_METHOD("is_configured"), &VoxelGrid::is_configured);
	ClassDB::bind_method(D_METHOD("get_voxel_size"), &VoxelGrid::get_voxel_size);
	ClassDB::bind_method(D_METHOD("get_dimensions"), &VoxelGrid::get_dimensions);
	ClassDB::bind_method(D_METHOD("set_out_of_bounds_occupied", "value"), &VoxelGrid::set_out_of_bounds_occupied);
	ClassDB::bind_method(D_METHOD("get_out_of_bounds_occupied"), &VoxelGrid::get_out_of_bounds_occupied);

	ClassDB::bind_method(D_METHOD("world_to_voxel", "world_pos"), &VoxelGrid::world_to_voxel);
	ClassDB::bind_method(D_METHOD("voxel_to_world", "voxel"), &VoxelGrid::voxel_to_world);
	ClassDB::bind_method(D_METHOD("coords_to_voxel", "coords"), &VoxelGrid::coords_to_voxel);
	ClassDB::bind_method(D_METHOD("voxel_to_coords", "voxel"), &VoxelGrid::voxel_to_coords);

	ClassDB::bind_method(D_METHOD("voxels_from_points", "points", "xform", "grow_by_voxels"), &VoxelGrid::voxels_from_points, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("has_overlap", "voxels"), &VoxelGrid::has_overlap);
	ClassDB::bind_method(D_METHOD("has_overlap_excluding", "voxels", "excluded"), &VoxelGrid::has_overlap_excluding);
	ClassDB::bind_method(D_METHOD("mark_occupied", "voxels"), &VoxelGrid::mark_occupied);
	ClassDB::bind_method(D_METHOD("mark_unoccupied", "voxels"), &VoxelGrid::mark_unoccupied);
	ClassDB::bind_method(D_METHOD("clear"), &VoxelGrid::clear);

	ClassDB::bind_method(D_METHOD("get_occupancy", "voxel"), &VoxelGrid::get_occupancy);
	ClassDB::bind_method(D_METHOD("get_occupied_voxels"), &VoxelGrid::get_occupied_voxels);
	ClassDB::bind_method(D_METHOD("get_occupied_count"), &VoxelGrid::get_occupied_count);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "out_of_bounds_occupied"),
			"set_out_of_bounds_occupied", "get_out_of_bounds_occupied");
}
