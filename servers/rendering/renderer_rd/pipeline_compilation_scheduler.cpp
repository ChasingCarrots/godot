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
#include "core/templates/local_vector.h"

namespace {

Mutex sources_mutex;
LocalVector<PipelineCompilationSource *> sources;

bool enabled = true;
uint32_t completed_count = 0;
uint32_t total_count = 0;

// How many compilations are allowed in flight. Never the whole pool: resource loading runs on the
// same WorkerThreadPool, and filling every worker with pipeline compiles made a mission load take
// 7.9 s instead of 4.4 s. During gameplay the frame needs those cores too, so it queues shallower.
constexpr uint32_t IN_FLIGHT_PLAYING = 2;

bool loading_screen = false;

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

void PipelineCompilationScheduler::set_loading_screen(bool p_enabled) {
	loading_screen = p_enabled;
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

void PipelineCompilationScheduler::tick() {
	if (!enabled) {
		return;
	}

	MutexLock lock(sources_mutex);
	if (sources.is_empty()) {
		return;
	}

	uint32_t reaped = 0;
	uint32_t in_flight = 0;
	uint32_t queued = 0;
	for (PipelineCompilationSource *source : sources) {
		reaped += source->harvest_pipelines();
		in_flight += source->in_flight_pipelines();
		queued += source->pending_pipelines();
	}
	completed_count += reaped;

	const uint32_t cap = loading_screen
			? MAX(1u, (uint32_t)WorkerThreadPool::get_singleton()->get_thread_count() / 2)
			: IN_FLIGHT_PLAYING;

	if (in_flight < cap) {
		uint32_t remaining = cap - in_flight;
		for (PipelineCompilationSource *source : sources) {
			if (remaining == 0) {
				break;
			}
			remaining -= source->submit_pending_pipelines(remaining);
		}
	}

	total_count = completed_count + queued + in_flight;
}
