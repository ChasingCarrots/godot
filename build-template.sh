#!/bin/bash
scons target=template_release debug_symbols=yes tools=no use_breakpad=yes production=yes lto=auto cache_path=./scons_cache/release