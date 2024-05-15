#include "ProfilerTracy.h"

#include <core/profiling.h>

void ProfilerTracy::startProfiling() {
	PROFILING_START();
}

void ProfilerTracy::_bind_methods() {
	ClassDB::bind_static_method("ProfilerTracy", D_METHOD("start_profiling"), &ProfilerTracy::startProfiling);
}