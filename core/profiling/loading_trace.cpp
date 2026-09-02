/**************************************************************************/
/*  loading_trace.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                        */
/**************************************************************************/

#include "loading_trace.h"

#include "core/object/worker_thread_pool.h"
#include "core/os/mutex.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

#ifdef _MSC_VER
// getenv/fopen are used deliberately: this writer must work from an atexit handler,
// after the engine's own filesystem and environment abstractions are gone.
#pragma warning(disable : 4996)
#endif

namespace {

const char *cat_names[LT_CAT_MAX] = {
	"res_request",
	"res_cache_hit",
	"res_task",
	"res_steal_wait",
	"res_cond_wait",
	"res_get",
	"sh_source",
	"sh_group",
	"sh_cache_load",
	"sh_variant",
	"sh_group_wait",
	"pso_submit",
	"pso_build",
	"pso_wait",
	"frame",
};

struct Rec {
	uint64_t start_us;
	uint64_t end_us; // 0 while in flight.
	uint64_t tid;
	uint32_t name;
	uint32_t detail;
	uint32_t phase_us; // Inner phase length, if the hook stamped one.
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t a4;
	int32_t wtp;
	uint8_t cat;
	bool is_instant;
};

// Heap-allocated and never freed: the exit flush must outlive every static destructor.
struct State {
	Mutex mutex;
	LocalVector<Rec> events;
	LocalVector<String> strings;
	HashMap<String, uint32_t> string_ids;
	String out_path;
	uint64_t epoch_us = 0;
	int64_t epoch_unix_ms = 0;
	uint64_t dropped = 0;
	uint32_t max_events = 1000000;
	bool flushed = false;
};

State *state = nullptr;

// Caller must hold the mutex.
uint32_t intern(const String &p_str) {
	if (p_str.is_empty()) {
		return 0xFFFFFFFF;
	}
	HashMap<String, uint32_t>::Iterator it = state->string_ids.find(p_str);
	if (it) {
		return it->value;
	}
	uint32_t id = state->strings.size();
	state->strings.push_back(p_str);
	state->string_ids.insert(p_str, id);
	return id;
}

void write_escaped(FILE *f, const String &p_str) {
	CharString utf8 = p_str.utf8();
	const char *c = utf8.get_data();
	for (; *c; c++) {
		switch (*c) {
			case '"':
				fputs("\\\"", f);
				break;
			case '\\':
				fputs("\\\\", f);
				break;
			case '\n':
				fputs("\\n", f);
				break;
			case '\r':
				fputs("\\r", f);
				break;
			case '\t':
				fputs("\\t", f);
				break;
			default:
				if ((unsigned char)*c < 0x20) {
					fprintf(f, "\\u%04x", (unsigned char)*c);
				} else {
					fputc(*c, f);
				}
		}
	}
}

void exit_flush() {
	LoadingTrace::flush();
}

} // namespace

bool LoadingTrace::_init() {
	const char *env = std::getenv("GODOT_LOADING_TRACE");
	if (env == nullptr || env[0] == 0) {
		return false;
	}

	state = new State();
	state->out_path = String::utf8(env);
	state->epoch_us = OS::get_singleton() ? OS::get_singleton()->get_ticks_usec() : 0;
	state->epoch_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch())
								   .count();

	const char *max_env = std::getenv("GODOT_LOADING_TRACE_MAX_EVENTS");
	if (max_env != nullptr && max_env[0] != 0) {
		int parsed = atoi(max_env);
		if (parsed > 0) {
			state->max_events = (uint32_t)parsed;
		}
	}

	state->events.reserve(1 << 16);
	// String id 0 is reserved so an empty detail can be distinguished from the first path.
	state->strings.push_back(String());
	state->string_ids.insert(String(), 0);

	atexit(&exit_flush);

	printf("LoadingTrace armed -> %s (epoch ticks_us=%llu unix_ms=%lld)\n",
			env, (unsigned long long)state->epoch_us, (long long)state->epoch_unix_ms);
	fflush(stdout);
	return true;
}

uint32_t LoadingTrace::begin(LoadingTraceCat p_cat, const String &p_name, const String &p_detail) {
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	const uint64_t tid = Thread::get_caller_id();
	WorkerThreadPool *wtp = WorkerThreadPool::get_singleton();
	const int32_t wtp_index = wtp ? wtp->get_thread_index() : -1;

	MutexLock lock(state->mutex);
	if (state->events.size() >= state->max_events) {
		state->dropped++;
		return INVALID_HANDLE;
	}

	Rec rec;
	rec.start_us = now;
	rec.end_us = 0;
	rec.tid = tid;
	rec.name = intern(p_name);
	rec.detail = intern(p_detail);
	rec.phase_us = 0;
	rec.a0 = 0;
	rec.a1 = 0;
	rec.a2 = 0;
	rec.a3 = 0;
	rec.a4 = 0;
	rec.wtp = wtp_index;
	rec.cat = (uint8_t)p_cat;
	rec.is_instant = false;

	uint32_t handle = state->events.size();
	state->events.push_back(rec);
	return handle;
}

void LoadingTrace::end(uint32_t p_handle) {
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	MutexLock lock(state->mutex);
	if (p_handle < state->events.size()) {
		state->events[p_handle].end_us = now;
	}
}

void LoadingTrace::set_args(uint32_t p_handle, uint32_t p_a0, uint32_t p_a1, uint32_t p_a2, uint32_t p_a3, uint32_t p_a4) {
	MutexLock lock(state->mutex);
	if (p_handle < state->events.size()) {
		Rec &rec = state->events[p_handle];
		rec.a0 = p_a0;
		rec.a1 = p_a1;
		rec.a2 = p_a2;
		rec.a3 = p_a3;
		rec.a4 = p_a4;
	}
}

void LoadingTrace::set_phase(uint32_t p_handle) {
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	MutexLock lock(state->mutex);
	if (p_handle < state->events.size()) {
		Rec &rec = state->events[p_handle];
		rec.phase_us = (uint32_t)MIN<uint64_t>(now - rec.start_us, 0xFFFFFFFF);
	}
}

void LoadingTrace::instant(LoadingTraceCat p_cat, const String &p_name, const String &p_detail, uint32_t p_a0) {
	if (!is_armed()) {
		return;
	}
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	const uint64_t tid = Thread::get_caller_id();
	WorkerThreadPool *wtp = WorkerThreadPool::get_singleton();
	const int32_t wtp_index = wtp ? wtp->get_thread_index() : -1;

	MutexLock lock(state->mutex);
	if (state->events.size() >= state->max_events) {
		state->dropped++;
		return;
	}

	Rec rec;
	rec.start_us = now;
	rec.end_us = now;
	rec.tid = tid;
	rec.name = intern(p_name);
	rec.detail = intern(p_detail);
	rec.phase_us = 0;
	rec.a0 = p_a0;
	rec.a1 = 0;
	rec.a2 = 0;
	rec.a3 = 0;
	rec.a4 = 0;
	rec.wtp = wtp_index;
	rec.cat = (uint8_t)p_cat;
	rec.is_instant = true;
	state->events.push_back(rec);
}

void LoadingTrace::flush() {
	if (state == nullptr) {
		return;
	}
	MutexLock lock(state->mutex);
	if (state->flushed) {
		return;
	}
	state->flushed = true;

	// The exit flush runs after the OS singleton is gone, so the tick clock is
	// unavailable here; derive the end stamp from the wall clock anchored at arm time.
	const int64_t now_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch())
										.count();
	const uint64_t flush_us = state->epoch_us + (uint64_t)MAX<int64_t>(0, (now_unix_ms - state->epoch_unix_ms) * 1000);

	FILE *f = fopen(state->out_path.utf8().get_data(), "wb");
	if (f == nullptr) {
		printf("LoadingTrace: could not open '%s' for writing.\n", state->out_path.utf8().get_data());
		return;
	}

	// Thread lanes, in order of first appearance, so Perfetto shows readable names.
	HashMap<uint64_t, uint32_t> lane_of_tid;
	LocalVector<int32_t> lane_wtp;
	for (const Rec &rec : state->events) {
		HashMap<uint64_t, uint32_t>::Iterator it = lane_of_tid.find(rec.tid);
		if (!it) {
			lane_of_tid.insert(rec.tid, lane_wtp.size());
			lane_wtp.push_back(rec.wtp);
		} else if (lane_wtp[it->value] < 0 && rec.wtp >= 0) {
			lane_wtp[it->value] = rec.wtp;
		}
	}

	fputs("{\"traceEvents\":[\n", f);
	fprintf(f, "{\"ph\":\"i\",\"s\":\"g\",\"cat\":\"meta\",\"name\":\"trace_epoch\",\"pid\":1,\"tid\":0,\"ts\":%llu,"
			   "\"args\":{\"epoch_ticks_us\":%llu,\"epoch_unix_ms\":%lld,\"end_ticks_us\":%llu,\"events\":%u,\"dropped\":%llu}},\n",
			(unsigned long long)state->epoch_us, (unsigned long long)state->epoch_us,
			(long long)state->epoch_unix_ms, (unsigned long long)flush_us,
			(unsigned)state->events.size(), (unsigned long long)state->dropped);

	const uint64_t main_tid = Thread::get_main_id();
	for (const KeyValue<uint64_t, uint32_t> &kv : lane_of_tid) {
		String label;
		if (kv.key == main_tid) {
			label = "Main";
		} else if (lane_wtp[kv.value] >= 0) {
			label = "WorkerThreadPool " + itos(lane_wtp[kv.value]);
		} else {
			label = "Thread " + String::num_uint64(kv.key);
		}
		fprintf(f, "{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":1,\"tid\":%u,\"args\":{\"name\":\"",
				(unsigned)kv.value);
		write_escaped(f, label);
		fprintf(f, "\",\"thread_id\":%llu}},\n", (unsigned long long)kv.key);
	}

	for (const Rec &rec : state->events) {
		const bool unfinished = !rec.is_instant && rec.end_us == 0;
		const uint64_t end_us = unfinished ? flush_us : rec.end_us;
		const uint32_t lane = lane_of_tid[rec.tid];

		if (rec.is_instant) {
			fprintf(f, "{\"ph\":\"i\",\"s\":\"t\",\"cat\":\"%s\",\"name\":\"", cat_names[rec.cat]);
		} else {
			fprintf(f, "{\"ph\":\"X\",\"cat\":\"%s\",\"name\":\"", cat_names[rec.cat]);
		}
		write_escaped(f, state->strings[rec.name == 0xFFFFFFFF ? (uint32_t)0 : rec.name]);
		fprintf(f, "\",\"pid\":1,\"tid\":%u,\"ts\":%llu", (unsigned)lane, (unsigned long long)rec.start_us);
		if (!rec.is_instant) {
			fprintf(f, ",\"dur\":%llu", (unsigned long long)(end_us - rec.start_us));
		}
		fputs(",\"args\":{", f);
		bool first = true;
		if (rec.detail != 0 && rec.detail != 0xFFFFFFFF) {
			fputs("\"detail\":\"", f);
			write_escaped(f, state->strings[rec.detail]);
			fputc('"', f);
			first = false;
		}
		if (rec.phase_us != 0) {
			fprintf(f, "%s\"phase_us\":%u", first ? "" : ",", rec.phase_us);
			first = false;
		}
		if (rec.a0 || rec.a1 || rec.a2 || rec.a3 || rec.a4) {
			fprintf(f, "%s\"a0\":%u,\"a1\":%u,\"a2\":%u,\"a3\":%u,\"a4\":%u",
					first ? "" : ",", rec.a0, rec.a1, rec.a2, rec.a3, rec.a4);
			first = false;
		}
		if (unfinished) {
			fprintf(f, "%s\"unfinished\":true", first ? "" : ",");
			first = false;
		}
		if (rec.wtp >= 0) {
			fprintf(f, "%s\"wtp\":%d", first ? "" : ",", rec.wtp);
		}
		fputs("}},\n", f);
	}

	// Every event line ends with a comma, so a run killed mid-write still parses line by line;
	// this trailing event carries no comma and closes the document, which Perfetto requires.
	fprintf(f, "{\"ph\":\"i\",\"s\":\"g\",\"cat\":\"meta\",\"name\":\"trace_end\",\"pid\":1,\"tid\":0,\"ts\":%llu}\n]}\n",
			(unsigned long long)flush_us);
	fclose(f);
	printf("LoadingTrace: wrote %u events (%llu dropped) to %s\n",
			(unsigned)state->events.size(), (unsigned long long)state->dropped,
			state->out_path.utf8().get_data());
	fflush(stdout);
}
