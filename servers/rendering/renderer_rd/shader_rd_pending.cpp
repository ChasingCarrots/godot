/**************************************************************************/
/*  shader_rd_pending.cpp                                                 */
/**************************************************************************/

#include "shader_rd.h"

#include "core/io/dir_access.h"
#include "core/object/worker_thread_pool.h"

// Separate translation unit so shader_rd.cpp stays untouched; see the fork's rule on keeping
// additions out of files that churn upstream.
//
// Non-blocking counterpart to version_get_shader(), which waits for the group it needs. Callers
// that must not stall (a boot screen that still has to draw) poll this instead.
bool ShaderRD::has_pending_group_compiles() {
	WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
	for (const RID &version_rid : version_owner.get_owned_list()) {
		Version *version = version_owner.get_or_null(version_rid);
		if (version == nullptr) {
			continue;
		}

		MutexLock lock(*version->mutex);
		for (int group = 0; group < version->group_compilation_tasks.size(); group++) {
			const WorkerThreadPool::GroupID task = version->group_compilation_tasks[group];
			if (task != 0 && !pool->is_group_task_completed(task)) {
				return true;
			}
		}
	}
	return false;
}

// A baked res:// shader cache is keyed by the engine's version hash: ShaderRD::setup() folds
// GODOT_VERSION_HASH into base_sha256, which every group hash is derived from. Exporting with an
// editor built from a different commit than the export template therefore ships a cache in which
// not one entry can ever be found, and nothing used to say so - the game just recompiled every
// variant on every machine's first launch, for minutes, while the pck carried the dead copy.
void ShaderRD::warn_if_res_cache_unusable() const {
	if (!shader_cache_res_dir_valid || group_sha256.is_empty()) {
		return;
	}

	const String shader_dir = shader_cache_res_dir.path_join(name);
	if (!DirAccess::exists(shader_dir)) {
		return; // This shader was never baked, which is a coverage question, not a mismatch.
	}

	for (const String &group_hash : group_sha256) {
		if (!group_hash.is_empty() && DirAccess::exists(shader_dir.path_join(group_hash))) {
			return;
		}
	}

	WARN_PRINT_ONCE(vformat("Baked shader cache in %s was produced by a different engine build than this one (first shader found stale: %s). None of it can be used, so every shader variant will be compiled at runtime. Re-export from an editor built at the same commit as the export template.", shader_cache_res_dir, name));
}
