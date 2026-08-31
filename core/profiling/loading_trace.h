/**************************************************************************/
/*  loading_trace.h                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                        */
/**************************************************************************/

#pragma once

#include "core/string/ustring.h"

// Always-compiled, opt-in tracing of resource loading and shader/pipeline
// compilation. Armed by setting the environment variable GODOT_LOADING_TRACE to an
// absolute output path; unarmed it costs one branch per hook point and nothing else.
//
// Events are buffered in RAM (no I/O while loading, so the trace does not perturb
// what it measures) and written at process exit as Chrome Trace Event JSON, one
// event per line. The document is closed properly so Perfetto accepts it; the line layout
// additionally lets an analyzer read a run that was killed mid-write.
//
// Optional: GODOT_LOADING_TRACE_MAX_EVENTS (default 1000000).

enum LoadingTraceCat : uint8_t {
	LT_RES_REQUEST, // load_threaded_request / _load_start: a load was asked for.
	LT_RES_CACHE_HIT, // Served from ResourceCache, no task ran.
	LT_RES_TASK, // _run_load_task body: the whole load of one file or sub-resource.
	LT_RES_STEAL_WAIT, // Awaiting thread ran a dependency task itself.
	LT_RES_COND_WAIT, // Awaiting thread blocked on a condition variable.
	LT_RES_GET, // Userland collected the result (includes main-thread spin).
	LT_SH_SOURCE, // Godot shader language -> GLSL.
	LT_SH_GROUP, // Variant group submitted for compilation.
	LT_SH_CACHE_LOAD, // Disk shader cache read + RID creation.
	LT_SH_VARIANT, // One variant: GLSL -> SPIR-V -> driver bytecode -> RID.
	LT_SH_GROUP_WAIT, // Stall waiting for a variant group.
	LT_PSO_SUBMIT, // Pipeline compilation queued.
	LT_PSO_BUILD, // Pipeline state object actually built.
	LT_PSO_WAIT, // Stall waiting for a pipeline.
	LT_CAT_MAX,
};

class LoadingTrace {
	static bool _init();

public:
	// Cheap armed check. The magic static runs _init() exactly once, on the first
	// hook point reached, and registers the exit flush.
	_FORCE_INLINE_ static bool is_armed() {
		static const bool armed = _init();
		return armed;
	}

	static constexpr uint32_t INVALID_HANDLE = 0xFFFFFFFF;

	static uint32_t begin(LoadingTraceCat p_cat, const String &p_name, const String &p_detail);
	static void end(uint32_t p_handle);
	static void set_args(uint32_t p_handle, uint32_t p_a0, uint32_t p_a1, uint32_t p_a2, uint32_t p_a3, uint32_t p_a4);
	static void set_phase(uint32_t p_handle); // Stamps elapsed usec as the span's inner phase.
	static void instant(LoadingTraceCat p_cat, const String &p_name, const String &p_detail, uint32_t p_a0);
	static void flush();
};

// RAII span. Zero-cost when the trace is not armed.
class LoadingTraceSpan {
	uint32_t handle = LoadingTrace::INVALID_HANDLE;

public:
	_FORCE_INLINE_ void args(uint32_t p_a0, uint32_t p_a1 = 0, uint32_t p_a2 = 0, uint32_t p_a3 = 0, uint32_t p_a4 = 0) {
		if (unlikely(handle != LoadingTrace::INVALID_HANDLE)) {
			LoadingTrace::set_args(handle, p_a0, p_a1, p_a2, p_a3, p_a4);
		}
	}

	// Marks how much of this span had elapsed at the call site.
	_FORCE_INLINE_ void phase() {
		if (unlikely(handle != LoadingTrace::INVALID_HANDLE)) {
			LoadingTrace::set_phase(handle);
		}
	}

	_FORCE_INLINE_ LoadingTraceSpan(LoadingTraceCat p_cat, const String &p_name, const String &p_detail = String()) {
		if (unlikely(LoadingTrace::is_armed())) {
			handle = LoadingTrace::begin(p_cat, p_name, p_detail);
		}
	}

	_FORCE_INLINE_ ~LoadingTraceSpan() {
		if (unlikely(handle != LoadingTrace::INVALID_HANDLE)) {
			LoadingTrace::end(handle);
		}
	}

	LoadingTraceSpan(const LoadingTraceSpan &) = delete;
	LoadingTraceSpan &operator=(const LoadingTraceSpan &) = delete;
};
