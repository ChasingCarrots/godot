/**************************************************************************/
/*  pso_record.h                                                          */
/**************************************************************************/

#pragma once

#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"

// Records which render pipelines a play session actually used, so a later run can build exactly
// those before they are first needed instead of compiling them inside a frame.
//
// A PipelineKey cannot be stored as-is: vertex_format_id and framebuffer_format_id are indices into
// content-keyed caches, assigned in first-use order, so they differ between runs. A record instead
// stores the *inputs* that produce them, and replay re-issues the same creation calls - which the
// caches answer with whatever ID this run uses, by construction the same one a draw would compute.
class PSORecord {
public:
	// Mirrors the clustered renderer's framebuffer-format helpers; the descriptor is what is
	// portable, the resulting ID is not.
	enum FBKind {
		FB_COLOR,
		FB_REFLECTION_PROBE_COLOR,
		FB_DEPTH,
		FB_SHADOW_CUBEMAP,
		FB_SHADOW_ATLAS,
		FB_REFLECTION_PROBE_DEPTH,
		FB_EMPTY,
		FB_MAX,
	};

	struct FBDesc {
		uint8_t kind = FB_MAX;
		uint8_t samples = 0;
		uint8_t flag_a = 0;
		uint8_t flag_b = 0;
		uint8_t view_count = 1;

		uint32_t pack() const {
			return uint32_t(kind) | (uint32_t(samples) << 4) | (uint32_t(flag_a) << 8) |
					(uint32_t(flag_b) << 9) | (uint32_t(view_count) << 10);
		}
		static FBDesc unpack(uint32_t p_packed) {
			FBDesc d;
			d.kind = p_packed & 0xF;
			d.samples = (p_packed >> 4) & 0xF;
			d.flag_a = (p_packed >> 8) & 0x1;
			d.flag_b = (p_packed >> 9) & 0x1;
			d.view_count = (p_packed >> 10) & 0x7;
			return d;
		}
	};

	struct Rec {
		uint64_t shader_hash = 0;
		uint64_t surface_format = 0;
		uint32_t fb = 0;
		uint32_t color_pass_flags = 0;
		uint32_t spec_0 = 0;
		uint32_t spec_1 = 0;
		uint32_t spec_2 = 0;
		uint8_t cull_mode = 0;
		uint8_t primitive = 0;
		uint8_t version = 0;
		uint8_t instanced = 0;
		uint8_t motion_vectors = 0;
		uint8_t point_size = 0;
		uint8_t wireframe = 0;
		uint8_t ubershader = 0;

		// Everything except the shader; two records differing only by shader are different
		// pipelines in different maps.
		uint64_t identity() const;
	};

	// Hot path: one predictable branch per surface when off.
	static _FORCE_INLINE_ bool is_armed() { return armed; }
	static void set_armed(bool p_armed);

	static void register_framebuffer_format(int64_t p_id, const FBDesc &p_desc);
	static bool framebuffer_desc_for_id(int64_t p_id, FBDesc &r_desc);
	static bool framebuffer_id_for_desc(const FBDesc &p_desc, int64_t &r_id);

	static void push(const Rec &p_rec);
	static void note_unknown_framebuffer(int64_t p_id);
	static uint32_t recorded_count();

	// Merges with whatever the file already holds: coverage accumulates over runs and over
	// testers, and a session that missed an area never deletes one that found it.
	static Error save(const String &p_path, uint32_t *r_total = nullptr);
	static Error load(const String &p_path, LocalVector<Rec> &r_recs);

	// Set by whichever renderer can rebuild keys; keeps this file free of renderer types.
	typedef uint32_t (*ReplayFunction)(const LocalVector<Rec> &p_recs, const Vector<RID> &p_materials, uint32_t *r_unmatched);
	static void set_replay_function(ReplayFunction p_function);
	static ReplayFunction get_replay_function();

private:
	static bool armed;
	static Mutex mutex;
	static HashMap<int64_t, uint32_t> fb_by_id;
	static HashMap<uint32_t, int64_t> id_by_fb;
	static HashSet<uint64_t> seen;
	static LocalVector<Rec> records;
	static ReplayFunction replay_function;
	static HashMap<int64_t, uint32_t> unknown_fb;
};
