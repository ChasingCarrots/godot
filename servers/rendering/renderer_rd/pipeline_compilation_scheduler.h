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
// A D3D12 driver compiling pipeline state objects on worker threads blocks the main thread's
// rendering for as long as the compilation runs: with a cold driver cache a mission load produced
// one 71 s frame, and lowering the worker count only made the stall longer. The work itself cannot
// be avoided, so it is cut into batches that are submitted and joined between frames. Nothing is
// ever in flight while a frame renders, so each frame gets through and a loading screen can show
// progress.
//
// Sources register themselves; the main loop calls tick() once per frame.
class PipelineCompilationSource {
public:
	virtual uint32_t pending_pipelines() const = 0;
	// Submits at most p_max deferred compilations; returns how many were started.
	virtual uint32_t submit_pending_pipelines(uint32_t p_max) = 0;
	virtual void join_submitted_pipelines() = 0;

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

	// A loading screen may spend most of its frame compiling; gameplay may not. The batch size
	// adapts to whatever time budget this selects, so leaving it on the loading value is what
	// turns a newly loaded mesh into a visible hitch.
	static void set_loading_screen(bool p_enabled);

	// Submits and joins one bounded batch. Main thread only, called between frames.
	static void tick();

	static uint32_t pending();
	static uint32_t completed();
	static uint32_t total();
};
