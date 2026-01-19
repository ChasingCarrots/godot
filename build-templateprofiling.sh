#!/bin/bash
scons target=template_release profiler=tracy profiler_path=~/Projects/tracy/ debug_symbols=yes tools=no use_breakpad=no production=yes cache_path=./scons_cache/profiling verbose=yes