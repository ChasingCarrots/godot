/**************************************************************************/
/*  pipeline_compilation_scheduler.cpp                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "pipeline_compilation_scheduler.h"

#include "core/config/project_settings.h"
#include "core/object/worker_thread_pool.h"
#include "core/os/mutex.h"
#include "core/os/os.h"
#include "core/profiling/loading_trace.h"
#include "core/templates/local_vector.h"

namespace {

Mutex sources_mutex;
LocalVector<PipelineCompilationSource *> sources;

bool enabled = true;
bool warmup = false;
uint32_t completed_count = 0;
uint32_t total_count = 0;

// Batch size adapts to hit this much main-thread time per frame. A single pipeline can exceed it
// on a cold driver cache and cannot be interrupted, so this is a target, not a guarantee.
uint32_t target_batch_usec = 100000;
uint32_t batch_size = 3;

} // namespace

void PipelineCompilationScheduler::register_source(PipelineCompilationSource *p_source) {
	MutexLock lock(sources_mutex);
	sources.push_back(p_source);
}

void PipelineCompilationScheduler::unregister_source(PipelineCompilationSource *p_source) {
	MutexLock lock(sources_mutex);
	for (uint32_t i = 0; i < sources.size(); i++) {
		if (sources[i] == p_source) {
			sources.remove_at_unordered(i);
			return;
		}
	}
}

bool PipelineCompilationScheduler::is_enabled() {
	return enabled;
}

void PipelineCompilationScheduler::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

bool PipelineCompilationScheduler::is_warmup() {
	return warmup;
}

void PipelineCompilationScheduler::set_warmup(bool p_enabled) {
	warmup = p_enabled;
}

uint32_t PipelineCompilationScheduler::pending() {
	MutexLock lock(sources_mutex);
	uint32_t count = 0;
	for (PipelineCompilationSource *source : sources) {
		count += source->pending_pipelines();
	}
	return count;
}

uint32_t PipelineCompilationScheduler::completed() {
	return completed_count;
}

uint32_t PipelineCompilationScheduler::total() {
	return total_count;
}

void PipelineCompilationScheduler::flush_priority() {
	if (!enabled) {
		return;
	}

	MutexLock lock(sources_mutex);
	uint32_t submitted = 0;
	for (PipelineCompilationSource *source : sources) {
		submitted += source->submit_priority_pipelines();
	}

	if (submitted == 0) {
		return;
	}

	LoadingTraceSpan _lt_flush(LT_PSO_WAIT, "flush_priority");
	_lt_flush.args(submitted);
	for (PipelineCompilationSource *source : sources) {
		source->join_submitted_pipelines();
	}
	completed_count += submitted;
}

void PipelineCompilationScheduler::tick() {
	if (!enabled) {
		return;
	}

	MutexLock lock(sources_mutex);
	if (sources.is_empty()) {
		return;
	}

	uint32_t remaining = MAX(batch_size, 1u);
	uint32_t submitted = 0;
	for (PipelineCompilationSource *source : sources) {
		if (remaining == 0) {
			break;
		}
		const uint32_t started = source->submit_pending_pipelines(remaining);
		submitted += started;
		remaining -= started;
	}

	if (submitted == 0) {
		return;
	}

	const uint64_t started_us = OS::get_singleton()->get_ticks_usec();
	{
		LoadingTraceSpan _lt_batch(LT_PSO_WAIT, "compile_batch");
		_lt_batch.args(submitted, batch_size);
		for (PipelineCompilationSource *source : sources) {
			source->join_submitted_pipelines();
		}
	}
	const uint64_t elapsed_us = MAX(OS::get_singleton()->get_ticks_usec() - started_us, (uint64_t)1);

	completed_count += submitted;
	total_count = completed_count + pending();

	// Feed back on the measured batch time. A batch compiles in parallel across the pool, so its
	// wall time is set by the slowest pipeline in it, not by how many there are: never go below one
	// per worker, or five cores idle while the frame waits on the sixth.
	const uint32_t min_batch = MAX(2, WorkerThreadPool::get_singleton()->get_thread_count() / 2);
	const double scale = (double)target_batch_usec / (double)elapsed_us;
	const double next = (double)batch_size * CLAMP(scale, 0.25, 1.5);
	batch_size = (uint32_t)CLAMP(next, (double)min_batch, 256.0);
}
