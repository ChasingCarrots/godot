/**************************************************************************/
/*  pipeline_compilation_scheduler.h                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/typedefs.h"

// Spreads pipeline compilation across frames.
//
// A D3D12 driver can take over a second to build one pipeline state object on a cold driver
// cache, and the work cannot be interrupted. What must never happen is the main thread waiting on
// it: on a 4-worker machine with a cold cache, joining every batch blocked the main thread for
// 96% of a 334 s boot - 366 frames over half a second, worst 3.6 s - while the worker pool sat
// 46% idle, because only one batch was ever in flight and nobody could queue the next one until
// the main thread had finished waiting for it.
//
// So the scheduler submits and *reaps*, never waits. Each tick harvests whatever finished since
// the last frame and tops the pool back up to a cap, and the frame in between is presented while
// the rest are still compiling. Draws never depend on this completing: a missing specialized
// pipeline falls back to its ubershader, which every measured run confirms (`DRAW`-sourced
// compilations stay at 0).
//
// Sources register themselves; the main loop calls tick() once per frame.
class PipelineCompilationSource {
public:
	// Queued but not yet handed to the worker pool.
	virtual uint32_t pending_pipelines() const = 0;
	// Submits at most p_max deferred compilations; returns how many were started.
	virtual uint32_t submit_pending_pipelines(uint32_t p_max) = 0;
	// Handed to the pool and not yet reaped.
	virtual uint32_t in_flight_pipelines() const = 0;
	// Reaps every finished compilation, leaves the rest running; returns how many were reaped.
	// Never blocks.
	virtual uint32_t harvest_pipelines() = 0;

protected:
	~PipelineCompilationSource() {}
};

class PipelineCompilationScheduler {
public:
	static void register_source(PipelineCompilationSource *p_source);
	static void unregister_source(PipelineCompilationSource *p_source);

	// True while deferral is active. When false, compile_pipeline() submits immediately, which is
	// the upstream behaviour and what a warm driver cache wants.
	static bool is_enabled();
	static void set_enabled(bool p_enabled);

	// How deep the pool is kept. A loading screen has nothing to lose by having every worker
	// compiling; gameplay shares those cores with the frame, so it queues shallowly.
	static void set_loading_screen(bool p_enabled);

	// Reaps finished compilations and tops the pool back up. Main thread only, once per frame.
	static void tick();

	static uint32_t pending();
	static uint32_t completed();
	static uint32_t total();
};
