/**************************************************************************/
/*  pso_record.cpp                                                        */
/**************************************************************************/

#include "pso_record.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/string/print_string.h"
#include "core/templates/hashfuncs.h"

bool PSORecord::armed = false;
Mutex PSORecord::mutex;
HashMap<int64_t, uint32_t> PSORecord::fb_by_id;
HashMap<uint32_t, int64_t> PSORecord::id_by_fb;
HashSet<uint64_t> PSORecord::seen;
LocalVector<PSORecord::Rec> PSORecord::records;
PSORecord::ReplayFunction PSORecord::replay_function = nullptr;
PSORecord::ReadyFunction PSORecord::ready_function = nullptr;
String PSORecord::cached_path;
LocalVector<PSORecord::Rec> PSORecord::cached_records;
HashMap<int64_t, uint32_t> PSORecord::unknown_fb;

const LocalVector<PSORecord::Rec> &PSORecord::load_cached(const String &p_path) {
	if (cached_path != p_path) {
		cached_records.clear();
		load(p_path, cached_records);
		cached_path = p_path;
	}
	return cached_records;
}

void PSORecord::note_unknown_framebuffer(int64_t p_id) {
	MutexLock lock(mutex);
	unknown_fb[p_id]++;
}

uint64_t PSORecord::Rec::identity() const {
	uint64_t h = hash_murmur3_one_64(shader_hash);
	h = hash_murmur3_one_64(surface_format, h);
	h = hash_murmur3_one_32(fb, h);
	h = hash_murmur3_one_32(color_pass_flags, h);
	h = hash_murmur3_one_32(spec_0, h);
	h = hash_murmur3_one_32(spec_1, h);
	h = hash_murmur3_one_32(spec_2, h);
	h = hash_murmur3_one_32(uint32_t(cull_mode) | (uint32_t(primitive) << 8) | (uint32_t(version) << 16), h);
	h = hash_murmur3_one_32(uint32_t(instanced) | (uint32_t(motion_vectors) << 1) | (uint32_t(point_size) << 2) |
					(uint32_t(wireframe) << 3) | (uint32_t(ubershader) << 4),
			h);
	return hash_fmix32(h);
}

void PSORecord::set_armed(bool p_armed) {
	armed = p_armed;
}

void PSORecord::register_framebuffer_format(int64_t p_id, const FBDesc &p_desc) {
	MutexLock lock(mutex);
	const uint32_t packed = p_desc.pack();
	// First writer wins: several descriptors can resolve to one format, and replay only needs one
	// call that reproduces it.
	if (!fb_by_id.has(p_id)) {
		fb_by_id[p_id] = packed;
	}
	id_by_fb[packed] = p_id;
}

bool PSORecord::framebuffer_desc_for_id(int64_t p_id, FBDesc &r_desc) {
	MutexLock lock(mutex);
	const uint32_t *packed = fb_by_id.getptr(p_id);
	if (packed == nullptr) {
		return false;
	}
	r_desc = FBDesc::unpack(*packed);
	return true;
}

bool PSORecord::framebuffer_id_for_desc(const FBDesc &p_desc, int64_t &r_id) {
	MutexLock lock(mutex);
	const int64_t *id = id_by_fb.getptr(p_desc.pack());
	if (id == nullptr) {
		return false;
	}
	r_id = *id;
	return true;
}

void PSORecord::push(const Rec &p_rec) {
	const uint64_t id = p_rec.identity();
	MutexLock lock(mutex);
	if (seen.has(id)) {
		return;
	}
	seen.insert(id);
	records.push_back(p_rec);
}

uint32_t PSORecord::recorded_count() {
	MutexLock lock(mutex);
	return records.size();
}

static Array _rec_to_array(const PSORecord::Rec &p_rec) {
	Array a;
	// Split, not stringified: a JSON number loses integer precision past 2^53, and parsing text
	// back saturates at INT64_MAX, which silently merges every hash with the top bit set.
	a.push_back(uint32_t(p_rec.shader_hash));
	a.push_back(uint32_t(p_rec.shader_hash >> 32));
	a.push_back(uint32_t(p_rec.surface_format));
	a.push_back(uint32_t(p_rec.surface_format >> 32));
	a.push_back(p_rec.fb);
	a.push_back(p_rec.color_pass_flags);
	a.push_back(p_rec.spec_0);
	a.push_back(p_rec.spec_1);
	a.push_back(p_rec.spec_2);
	a.push_back(p_rec.cull_mode);
	a.push_back(p_rec.primitive);
	a.push_back(p_rec.version);
	a.push_back(p_rec.instanced);
	a.push_back(p_rec.motion_vectors);
	a.push_back(p_rec.point_size);
	a.push_back(p_rec.wireframe);
	a.push_back(p_rec.ubershader);
	return a;
}

static bool _array_to_rec(const Array &p_array, PSORecord::Rec &r_rec) {
	if (p_array.size() != 17) {
		return false;
	}
	r_rec.shader_hash = uint64_t(uint32_t(p_array[0])) | (uint64_t(uint32_t(p_array[1])) << 32);
	r_rec.surface_format = uint64_t(uint32_t(p_array[2])) | (uint64_t(uint32_t(p_array[3])) << 32);
	r_rec.fb = p_array[4];
	r_rec.color_pass_flags = p_array[5];
	r_rec.spec_0 = p_array[6];
	r_rec.spec_1 = p_array[7];
	r_rec.spec_2 = p_array[8];
	r_rec.cull_mode = p_array[9];
	r_rec.primitive = p_array[10];
	r_rec.version = p_array[11];
	r_rec.instanced = p_array[12];
	r_rec.motion_vectors = p_array[13];
	r_rec.point_size = p_array[14];
	r_rec.wireframe = p_array[15];
	r_rec.ubershader = p_array[16];
	return true;
}

Error PSORecord::load(const String &p_path, LocalVector<Rec> &r_recs) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ERR_FILE_CANT_OPEN;
	}

	JSON json;
	const Error err = json.parse(f->get_as_text());
	if (err != OK) {
		ERR_PRINT(vformat("PSO records %s: %s at line %d", p_path, json.get_error_message(), json.get_error_line()));
		return err;
	}

	const Array list = Dictionary(json.get_data()).get("records", Array());
	for (int i = 0; i < list.size(); i++) {
		Rec rec;
		if (_array_to_rec(list[i], rec)) {
			r_recs.push_back(rec);
		}
	}
	return OK;
}

Error PSORecord::save(const String &p_path, uint32_t *r_total) {
	LocalVector<Rec> merged;
	load(p_path, merged);

	HashSet<uint64_t> ids;
	for (const Rec &rec : merged) {
		ids.insert(rec.identity());
	}

	{
		MutexLock lock(mutex);
		for (const Rec &rec : records) {
			if (!ids.has(rec.identity())) {
				ids.insert(rec.identity());
				merged.push_back(rec);
			}
		}
	}

	Array out;
	for (const Rec &rec : merged) {
		out.push_back(_rec_to_array(rec));
	}
	Dictionary root;
	root["version"] = 2;
	root["records"] = out;

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_WRITE, vformat("Cannot write PSO records to %s", p_path));
	f->store_string(JSON::stringify(root));

	if (r_total != nullptr) {
		*r_total = merged.size();
	}


	{
		MutexLock lock(mutex);
		if (!unknown_fb.is_empty()) {
			String ids;
			uint32_t dropped = 0;
			for (const KeyValue<int64_t, uint32_t> &kv : unknown_fb) {
				ids += vformat(" %d(x%d)", kv.key, kv.value);
				dropped += kv.value;
			}
			WARN_PRINT(vformat("PSO recording dropped %d draws using %d unrecognised framebuffer formats:%s", dropped, unknown_fb.size(), ids));
		}
	}
	return OK;
}

void PSORecord::set_replay_function(ReplayFunction p_function) {
	replay_function = p_function;
}

PSORecord::ReplayFunction PSORecord::get_replay_function() {
	return replay_function;
}

void PSORecord::set_ready_function(ReadyFunction p_function) {
	ready_function = p_function;
}

bool PSORecord::shaders_ready() {
	return ready_function == nullptr || ready_function();
}
