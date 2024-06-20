#include "ProfilerTracy.h"

#include <core/os/time.h>
#include <core/profiling.h>

void ProfilerTracy::startProfiling(const String& name) {
	PROFILING_START_CAPTURE(name);
}

void ProfilerTracy::_bind_methods() {
	ClassDB::bind_static_method("ProfilerTracy", D_METHOD("start_profiling", "file_name"), &ProfilerTracy::startProfiling);
}