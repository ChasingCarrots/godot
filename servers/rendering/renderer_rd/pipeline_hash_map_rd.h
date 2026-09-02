/**************************************************************************/
/*  pipeline_hash_map_rd.h                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/object/worker_thread_pool.h"
#include "core/os/mutex.h"
#include "core/profiling/loading_trace.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/rendering/renderer_rd/pipeline_compilation_scheduler.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_enums.h"

#define PRINT_PIPELINE_COMPILATION_KEYS 0

// PIPELINE_SOURCE_DRAW means the pipeline was missing when a draw needed it, i.e. warm-up
// failed to predict it. Recording the source is what makes that measurable.
_FORCE_INLINE_ const char *pipeline_source_name(RSE::PipelineSource p_source) {
	switch (p_source) {
		case RSE::PIPELINE_SOURCE_CANVAS:
			return "CANVAS";
		case RSE::PIPELINE_SOURCE_MESH:
			return "MESH";
		case RSE::PIPELINE_SOURCE_SURFACE:
			return "SURFACE";
		case RSE::PIPELINE_SOURCE_DRAW:
			return "DRAW";
		case RSE::PIPELINE_SOURCE_SPECIALIZATION:
			return "SPECIALIZATION";
		default:
			return "UNKNOWN";
	}
}

template <typename Key, typename CreationClass, typename CreationFunction>
class PipelineHashMapRD : public PipelineCompilationSource {
private:
	CreationClass *creation_object = nullptr;
	CreationFunction creation_function = nullptr;
	Mutex *compilations_mutex = nullptr;
	uint32_t *compilations = nullptr;
	RBMap<uint32_t, RID> hash_map;
	LocalVector<Pair<uint32_t, RID>> compiled_queue;
	Mutex compiled_queue_mutex;
	RBSet<uint32_t> compilation_set;
	HashMap<uint32_t, WorkerThreadPool::TaskID> compilation_tasks;
	Mutex local_mutex;

	struct Deferred {
		Key key;
		uint32_t hash = 0;
	};

	// Compilations registered but not yet handed to the pool; the scheduler releases them in
	// batches so no compilation is ever in flight while a frame renders. Ubershader pipelines go in
	// the priority queue: they are the fallback a draw falls back *to*, so until one exists the
	// draw has to compile it inline and stall the frame.
	LocalVector<Deferred> deferred;
	LocalVector<Deferred> deferred_priority;
	LocalVector<WorkerThreadPool::TaskID> batch_tasks;

	bool _add_new_pipelines_to_map() {
		thread_local Vector<uint32_t> hashes_added;
		hashes_added.clear();

		{
			MutexLock lock(compiled_queue_mutex);
			for (const Pair<uint32_t, RID> &pair : compiled_queue) {
				if (hash_map.find(pair.first) != nullptr) {
					// A blocked draw already compiled this key inline; the queued task raced it and
					// produced a duplicate that was never handed out.
					if (pair.second.is_valid()) {
						RD::get_singleton()->free_rid(pair.second);
					}
					continue;
				}

				hash_map[pair.first] = pair.second;
				hashes_added.push_back(pair.first);
			}

			compiled_queue.clear();
		}

		{
			MutexLock local_lock(local_mutex);
			for (uint32_t hash : hashes_added) {
				HashMap<uint32_t, WorkerThreadPool::TaskID>::Iterator task_it = compilation_tasks.find(hash);
				if (task_it != compilation_tasks.end()) {
					compilation_tasks.remove(task_it);
				}
			}
		}

		return !hashes_added.is_empty();
	}

	void _wait_for_all_pipelines() {
		thread_local LocalVector<WorkerThreadPool::TaskID> tasks_to_wait;
		tasks_to_wait.clear();
		{
			MutexLock local_lock(local_mutex);
			for (KeyValue<uint32_t, WorkerThreadPool::TaskID> key_value : compilation_tasks) {
				tasks_to_wait.push_back(key_value.value);
			}
		}

		if (tasks_to_wait.is_empty()) {
			return;
		}

		// Joining every outstanding compile. Cold, this is tens of seconds of main-thread stall,
		// so it must be visible in a trace rather than showing up as an unexplained frame gap.
		LoadingTraceSpan _lt_wait(LT_PSO_WAIT, "clear_pipelines");
		_lt_wait.args((uint32_t)tasks_to_wait.size());
		for (WorkerThreadPool::TaskID task_id : tasks_to_wait) {
			WorkerThreadPool::get_singleton()->wait_for_task_completion(task_id);
		}
	}

	// Compile on the calling thread. Waiting for the queued task instead can stall for seconds,
	// because it sits behind every pipeline the loading meshes have already submitted.
	void _compile_pipeline_inline(const Key &p_key, uint32_t p_key_hash, RSE::PipelineSource p_source) {
		DEV_ASSERT((creation_object != nullptr) && (creation_function != nullptr) && "Creation object and function was not set before attempting to compile a pipeline.");

		{
			MutexLock local_lock(local_mutex);
			if (!compilation_set.has(p_key_hash)) {
				compilation_set.insert(p_key_hash);

				if (compilations_mutex != nullptr) {
					MutexLock compilations_lock(*compilations_mutex);
					compilations[p_source]++;
				}

				if (unlikely(LoadingTrace::is_armed())) {
					LoadingTrace::instant(LT_PSO_SUBMIT, pipeline_source_name(p_source), String(), p_key_hash);
				}
			}
		}

		(creation_object->*creation_function)(p_key);
	}

	// Moves a key into the priority queue so the very next batch builds it.
	void _promote_pending(uint32_t p_key_hash) {
		MutexLock local_lock(local_mutex);
		for (uint32_t i = 0; i < deferred.size(); i++) {
			if (deferred[i].hash == p_key_hash) {
				deferred_priority.push_back(deferred[i]);
				deferred.remove_at_unordered(i);
				return;
			}
		}
	}

public:
	void add_compiled_pipeline(uint32_t p_hash, RID p_pipeline) {
		compiled_queue_mutex.lock();
		compiled_queue.push_back({ p_hash, p_pipeline });
		compiled_queue_mutex.unlock();
	}

	// Start compilation of a pipeline ahead of time in the background. Returns true if the compilation was started, false if it wasn't required. Source is only used for collecting statistics.
	void compile_pipeline(const Key &p_key, uint32_t p_key_hash, RSE::PipelineSource p_source, bool p_high_priority) {
		DEV_ASSERT((creation_object != nullptr) && (creation_function != nullptr) && "Creation object and function was not set before attempting to compile a pipeline.");

		MutexLock local_lock(local_mutex);
		if (compilation_set.has(p_key_hash)) {
			// Check if the pipeline was already submitted.
			return;
		}

		// Record the pipeline as submitted, a task can't be started for it again.
		compilation_set.insert(p_key_hash);

		if (compilations_mutex != nullptr) {
			MutexLock compilations_lock(*compilations_mutex);
			compilations[p_source]++;
		}

#if PRINT_PIPELINE_COMPILATION_KEYS
		String source_name = "UNKNOWN";
		switch (p_source) {
			case RSE::PIPELINE_SOURCE_CANVAS:
				source_name = "CANVAS";
				break;
			case RSE::PIPELINE_SOURCE_MESH:
				source_name = "MESH";
				break;
			case RSE::PIPELINE_SOURCE_SURFACE:
				source_name = "SURFACE";
				break;
			case RSE::PIPELINE_SOURCE_DRAW:
				source_name = "DRAW";
				break;
			case RSE::PIPELINE_SOURCE_SPECIALIZATION:
				source_name = "SPECIALIZATION";
				break;
		}

		print_line("HASH:", p_key_hash, "SOURCE:", source_name);
#endif

		if (unlikely(LoadingTrace::is_armed())) {
			LoadingTrace::instant(LT_PSO_SUBMIT, pipeline_source_name(p_source), String(), p_key_hash);
		}

		if (PipelineCompilationScheduler::is_enabled()) {
			// Hand it to the scheduler instead. Compiling while a frame renders stalls the driver,
			// so batches are released between frames.
			Deferred entry;
			entry.key = p_key;
			entry.hash = p_key_hash;
			// p_high_priority is set for ubershader pipelines at the submission sites.
			if (p_high_priority) {
				deferred_priority.push_back(entry);
			} else {
				deferred.push_back(entry);
			}
			return;
		}

		// Queue a background compilation task.
		WorkerThreadPool::TaskID task_id = WorkerThreadPool::get_singleton()->add_template_task(creation_object, creation_function, p_key, p_high_priority, "PipelineCompilation");
		compilation_tasks.insert(p_key_hash, task_id);
	}

	void wait_for_pipeline(uint32_t p_key_hash) {
		WorkerThreadPool::TaskID task_id_to_wait = WorkerThreadPool::INVALID_TASK_ID;

		{
			MutexLock local_lock(local_mutex);
			if (!compilation_set.has(p_key_hash)) {
				// The pipeline was never submitted, we can't wait for it.
				return;
			}

			HashMap<uint32_t, WorkerThreadPool::TaskID>::Iterator task_it = compilation_tasks.find(p_key_hash);
			if (task_it != compilation_tasks.end()) {
				// Wait for and remove the compilation task if it exists.
				task_id_to_wait = task_it->value;
				compilation_tasks.remove(task_it);
			}
		}

		if (task_id_to_wait != WorkerThreadPool::INVALID_TASK_ID) {
			LoadingTraceSpan _lt_wait(LT_PSO_WAIT, "pipeline");
			_lt_wait.args(p_key_hash);
			WorkerThreadPool::get_singleton()->wait_for_task_completion(task_id_to_wait);
		}
	}

	// Retrieve a pipeline. It'll return an empty pipeline if it's not available yet, but it'll be guaranteed to succeed if 'wait for compilation' is true and stall as necessary. Source is just an optional number to aid debugging.
	RID get_pipeline(const Key &p_key, uint32_t p_key_hash, bool p_wait_for_compilation, RSE::PipelineSource p_source) {
		RBMap<uint32_t, RID>::Element *e = hash_map.find(p_key_hash);

		if (e == nullptr) {
			// Check if there's any new pipelines that need to be added and try again. This method triggers a mutex lock.
			if (_add_new_pipelines_to_map()) {
				e = hash_map.find(p_key_hash);
			}
		}

		if (e == nullptr) {
			if (p_wait_for_compilation) {
				// Callers of the waiting form have no fallback left, so this must return a pipeline.
				// Promote first so the next inter-frame batch builds it and later frames find it
				// ready here instead of paying the inline compile again.
				_promote_pending(p_key_hash);

				// Canvas is exempt: the boot screen itself is 2D, and skipping those draws would
				// blank the progress UI this mode exists to keep responsive.
				if (PipelineCompilationScheduler::is_warmup() && p_source != RSE::PIPELINE_SOURCE_CANVAS) {
					// Warm-up geometry is throwaway: let the draw be skipped rather than stalling
					// the main thread, and let the batch scheduler compile this in the background.
					return RID();
				}

				// A draw is blocked on this pipeline, so build it here rather than queueing it.
				_compile_pipeline_inline(p_key, p_key_hash, p_source);
				_add_new_pipelines_to_map();

				e = hash_map.find(p_key_hash);
				if (e != nullptr) {
					return e->value();
				} else {
					// Pipeline could not be compiled due to an internal error. Store an empty RID so compilation is not attempted again.
					hash_map[p_key_hash] = RID();
					return RID();
				}
			} else {
				// Request compilation. The method will ignore the request if it's already being compiled.
				compile_pipeline(p_key, p_key_hash, p_source, false);
				return RID();
			}
		} else {
			return e->value();
		}
	}

	// Delete all cached pipelines. Can stall if background compilation is in progress.
	void clear_pipelines() {
		{
			// Anything still deferred targets pipelines that are about to be discarded.
			MutexLock local_lock(local_mutex);
			deferred.clear();
		}
		_wait_for_all_pipelines();
		_add_new_pipelines_to_map();

		for (KeyValue<uint32_t, RID> entry : hash_map) {
			RD::get_singleton()->free_rid(entry.value);
		}

		hash_map.clear();
		compilation_set.clear();
	}

	// Set the external pipeline compilations array to increase the counters on every time a pipeline is compiled.
	void set_compilations(uint32_t *p_compilations, Mutex *p_compilations_mutex) {
		compilations = p_compilations;
		compilations_mutex = p_compilations_mutex;
	}

	void set_creation_object_and_function(CreationClass *p_creation_object, CreationFunction p_creation_function) {
		creation_object = p_creation_object;
		creation_function = p_creation_function;
	}

	uint32_t pending_pipelines() const override {
		return deferred.size() + deferred_priority.size();
	}

	uint32_t submit_pending_pipelines(uint32_t p_max) override {
		uint32_t started = 0;
		MutexLock local_lock(local_mutex);
		while (started < p_max) {
			LocalVector<Deferred> &queue = deferred_priority.is_empty() ? deferred : deferred_priority;
			if (queue.is_empty()) {
				break;
			}

			const Deferred entry = queue[queue.size() - 1];
			queue.remove_at(queue.size() - 1);
			WorkerThreadPool::TaskID task_id = WorkerThreadPool::get_singleton()->add_template_task(creation_object, creation_function, entry.key, true, "PipelineCompilation");
			compilation_tasks.insert(entry.hash, task_id);
			batch_tasks.push_back(task_id);
			started++;
		}
		return started;
	}

	uint32_t submit_priority_pipelines() override {
		uint32_t started = 0;
		MutexLock local_lock(local_mutex);
		while (!deferred_priority.is_empty()) {
			const Deferred entry = deferred_priority[deferred_priority.size() - 1];
			deferred_priority.remove_at(deferred_priority.size() - 1);
			WorkerThreadPool::TaskID task_id = WorkerThreadPool::get_singleton()->add_template_task(creation_object, creation_function, entry.key, true, "PipelineCompilation");
			compilation_tasks.insert(entry.hash, task_id);
			batch_tasks.push_back(task_id);
			started++;
		}
		return started;
	}

	void join_submitted_pipelines() override {
		for (WorkerThreadPool::TaskID task_id : batch_tasks) {
			WorkerThreadPool::get_singleton()->wait_for_task_completion(task_id);
		}
		batch_tasks.clear();

		// The IDs are now freed; leaving them in compilation_tasks would let clear_pipelines()
		// wait on them a second time, which errors with "Invalid Task ID".
		MutexLock local_lock(local_mutex);
		compilation_tasks.clear();
	}

	PipelineHashMapRD() {
		PipelineCompilationScheduler::register_source(this);
	}

	~PipelineHashMapRD() {
		PipelineCompilationScheduler::unregister_source(this);
		clear_pipelines();
	}
};
