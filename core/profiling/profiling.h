/**************************************************************************/
/*  profiling.h                                                           */
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

#include "core/profiling/profiling.gen.h" // IWYU pragma: keep.

// This header provides profiling primitives (implemented as macros) for various backends.
// See the "No profiling" branch at the bottom for a short description of the functions.

// To configure / use the profiler, use the --profiler_path and other --profiler_* arguments
// when compiling Godot. You can also find details in the SCSub file (in this folder).

// Note: It is highly recommended to avoid including this header in other header files.
//       Prefer including it in .cpp files only. The reason is that we want to keep
//       the recompile cost of changing the profiler as low as possible.

#if defined(GODOT_USE_TRACY)
// Use the tracy profiler.

#include "core/string/string_name.h"

#define TRACY_ENABLE

GODOT_MSVC_WARNING_PUSH_AND_IGNORE(4366) // The result of the unary operator may be unaligned.
#include <tracy/Tracy.hpp>
#include <common/TracySystem.hpp>
#include <tracy/TracyC.h>
GODOT_MSVC_WARNING_POP

// Hijacking the tracy namespace so we can use their macros.
namespace tracy {
const SourceLocationData *intern_source_location(const void *p_function_ptr, const StringName &p_file, const StringName &p_function, const StringName &p_name, uint32_t p_line, bool p_is_script);
} //namespace tracy

// Define tracing macros.
#define GodotProfileFrameMark FrameMark
#define GodotProfileFunction() ZoneNamedC(___tracy_scoped_zone, 0x07631F, TracyCIsConnected)

#define GodotProfileZone(m_zone_name) ZoneNamedN(GD_UNIQUE_NAME(__godot_tracy_szone_), m_zone_name, TracyCIsConnected)
#define GodotProfileZoneNamed(m_var_name, m_zone_name) ZoneNamedN(m_var_name, m_zone_name, TracyCIsConnected)
#define GodotProfileZoneName(m_var_name, m_txt, m_size) m_var_name.Name(m_txt, m_size)

#define GodotProfileZoneGroupedFirst(m_group_name, m_zone_name) \
bool __godot_tracy_connected_##m_group_name = TracyCIsConnected; \
ZoneNamedN(__godot_tracy_zone_##m_group_name, m_zone_name, __godot_tracy_connected_##m_group_name)

#define GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name) \
if (__godot_tracy_connected_##m_group_name) { \
__godot_tracy_zone_##m_group_name.~ScopedZone(); \
}

#define GodotProfileDynamicZoneStart(NAME)
#define GodotProfileDynamicZoneEnd()
#ifndef TRACY_CALLSTACK
#define GodotProfileZoneGrouped(m_group_name, m_zone_name) \
GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name); \
if (__godot_tracy_connected_##m_group_name) { \
static constexpr tracy::SourceLocationData TracyConcat(__tracy_source_location, TracyLine){ m_zone_name, TracyFunction, TracyFile, (uint32_t)TracyLine, 0 }; \
new (&__godot_tracy_zone_##m_group_name) tracy::ScopedZone(&TracyConcat(__tracy_source_location, TracyLine), true); \
}
#else
#define GodotProfileZoneGrouped(m_group_name, m_zone_name) \
GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name); \
if (__godot_tracy_connected_##m_group_name) { \
static constexpr tracy::SourceLocationData TracyConcat(__tracy_source_location, TracyLine){ m_zone_name, TracyFunction, TracyFile, (uint32_t)TracyLine, 0 }; \
new (&__godot_tracy_zone_##m_group_name) tracy::ScopedZone(&TracyConcat(__tracy_source_location, TracyLine), TRACY_CALLSTACK, true); \
}

#endif

#define GodotProfileZoneScript(m_ptr, m_file, m_function, m_name, m_line) \
	tracy::ScopedZone __godot_tracy_script(tracy::intern_source_location(m_ptr, m_file, m_function, m_name, m_line, true), TRACY_CALLSTACK, TracyCIsConnected)
#define GodotProfileZoneScriptSystemCall(m_ptr, m_file, m_function, m_name, m_line) \
	tracy::ScopedZone __godot_tracy_zone_system_call(tracy::intern_source_location(m_ptr, m_file, m_function, m_name, m_line, false), TRACY_CALLSTACK, TracyCIsConnected)

// Memory allocation
#ifdef GODOT_PROFILER_TRACK_MEMORY
#define GodotProfileAlloc(m_ptr, m_size) \
	GODOT_GCC_WARNING_PUSH_AND_IGNORE("-Wmaybe-uninitialized") \
	TracyAlloc(m_ptr, m_size); \
	GODOT_GCC_WARNING_POP
#define GodotProfileFree(m_ptr) TracyFree(m_ptr)
#else
#define GodotProfileAlloc(m_ptr, m_size)
#define GodotProfileFree(m_ptr)
#endif

void godot_init_profiler();
void godot_cleanup_profiler();

#define GodotProfilingThreadStart(NAME)
#define GodotProfileSendScreenshot()
#define GodotProfileHasGPU() true

#ifdef GODOT_PROFILING_D3D12
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

GODOT_MSVC_WARNING_PUSH_AND_IGNORE(4366)
#include <tracy/TracyD3D12.hpp>
GODOT_MSVC_WARNING_POP

#include <cstring>

struct GodotTracyD3D12ZoneStack {
	struct ZoneSlot {
		alignas(tracy::D3D12ZoneScope) uint8_t storage[sizeof(tracy::D3D12ZoneScope)];
	};
	LocalVector<ZoneSlot> slots;

	GodotTracyD3D12ZoneStack() {
		slots.reserve(32);
	}

	~GodotTracyD3D12ZoneStack() {
		end_all();
	}

	_FORCE_INLINE_ void begin(void *p_ctx, ID3D12GraphicsCommandList *p_cmd_list, const char *p_name, const char *p_file, const char *p_function, uint32_t p_line) {
		if (!p_ctx || !p_cmd_list) {
			return;
		}
		const char *name = p_name ? p_name : "";
		const uint32_t idx = slots.size();
		slots.resize(idx + 1);
		new (slots[idx].storage) tracy::D3D12ZoneScope(static_cast<TracyD3D12Ctx>(p_ctx), p_line, p_file, strlen(p_file), p_function, strlen(p_function), name, strlen(name), p_cmd_list, true);
	}

	_FORCE_INLINE_ void end() {
		if (slots.is_empty()) {
			return;
		}
		const uint32_t idx = slots.size() - 1;
		reinterpret_cast<tracy::D3D12ZoneScope *>(slots[idx].storage)->~D3D12ZoneScope();
		slots.resize(idx);
	}

	_FORCE_INLINE_ void end_all() {
		while (!slots.is_empty()) {
			end();
		}
	}
};

struct GodotTracyD3D12State {
	Mutex mutex;
	TracyD3D12Ctx ctx = nullptr;
	ID3D12CommandQueue *main_queue = nullptr;
	HashMap<ID3D12GraphicsCommandList *, GodotTracyD3D12ZoneStack *> cmd_stacks;

	static GodotTracyD3D12State &singleton() {
		static GodotTracyD3D12State state;
		return state;
	}
};

_FORCE_INLINE_ void GodotProfileD3D12Init(ID3D12Device *p_device, ID3D12CommandQueue *p_queue) {
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (state.ctx != nullptr) {
		return;
	}
	state.main_queue = p_queue;
	state.ctx = TracyD3D12Context(p_device, p_queue);
	if (state.ctx) {
		TracyD3D12ContextName(state.ctx, "D3D12", 5);
	}
}

_FORCE_INLINE_ void GodotProfileD3D12Destroy() {
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	for (KeyValue<ID3D12GraphicsCommandList *, GodotTracyD3D12ZoneStack *> &E : state.cmd_stacks) {
		memdelete(E.value);
	}
	state.cmd_stacks.clear();
	if (state.ctx) {
		TracyD3D12Destroy(state.ctx);
		state.ctx = nullptr;
	}
	state.main_queue = nullptr;
}

_FORCE_INLINE_ void GodotProfileD3D12NewFrame(ID3D12CommandQueue *p_queue) {
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (state.ctx && p_queue == state.main_queue) {
		TracyD3D12NewFrame(state.ctx);
	}
}

_FORCE_INLINE_ void GodotProfileD3D12Collect() {
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (state.ctx) {
		TracyD3D12Collect(state.ctx);
	}
}

_FORCE_INLINE_ void GodotProfileD3D12ZoneBeginImpl(ID3D12GraphicsCommandList *p_cmd_list, const char *p_name, const char *p_file, const char *p_function, uint32_t p_line) {
	if (!p_cmd_list) {
		return;
	}
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (!state.ctx) {
		return;
	}
	GodotTracyD3D12ZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_list);
	GodotTracyD3D12ZoneStack *stack = stack_ptr ? *stack_ptr : nullptr;
	if (!stack) {
		stack = memnew(GodotTracyD3D12ZoneStack);
		state.cmd_stacks.insert(p_cmd_list, stack);
	}
	stack->begin(state.ctx, p_cmd_list, p_name, p_file, p_function, p_line);
}

#define GodotProfileD3D12ZoneBegin(m_cmd_list, m_name) \
	GodotProfileD3D12ZoneBeginImpl(m_cmd_list, m_name, TracyFile, TracyFunction, (uint32_t)TracyLine)

_FORCE_INLINE_ void GodotProfileD3D12ZoneEnd(ID3D12GraphicsCommandList *p_cmd_list) {
	if (!p_cmd_list) {
		return;
	}
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyD3D12ZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_list)) {
		(*stack_ptr)->end();
	}
}

_FORCE_INLINE_ void GodotProfileD3D12ZoneEndAll(ID3D12GraphicsCommandList *p_cmd_list) {
	if (!p_cmd_list) {
		return;
	}
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyD3D12ZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_list)) {
		(*stack_ptr)->end_all();
	}
}

_FORCE_INLINE_ void GodotProfileD3D12FreeCmdList(ID3D12GraphicsCommandList *p_cmd_list) {
	if (!p_cmd_list) {
		return;
	}
	GodotTracyD3D12State &state = GodotTracyD3D12State::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyD3D12ZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_list)) {
		memdelete(*stack_ptr);
		state.cmd_stacks.erase(p_cmd_list);
	}
}
#endif // GODOT_PROFILING_D3D12

#ifdef GODOT_PROFILING_VULKAN
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

#define TRACY_VK_USE_SYMBOL_TABLE
GODOT_MSVC_WARNING_PUSH_AND_IGNORE(4366)
#include <tracy/TracyVulkan.hpp>
GODOT_MSVC_WARNING_POP

#include <cstring>

struct GodotTracyVkZoneStack {
	struct ZoneSlot {
		alignas(tracy::VkCtxScope) uint8_t storage[sizeof(tracy::VkCtxScope)];
	};
	LocalVector<ZoneSlot> slots;
	bool is_primary = false;

	GodotTracyVkZoneStack() {
		slots.reserve(32);
	}

	~GodotTracyVkZoneStack() {
		end_all();
	}

	_FORCE_INLINE_ void begin(void *p_ctx, VkCommandBuffer p_cmd_buf, const char *p_name, const char *p_file, const char *p_function, uint32_t p_line) {
		if (!p_ctx || p_cmd_buf == VK_NULL_HANDLE) {
			return;
		}
		const char *name = p_name ? p_name : "";
		const uint32_t idx = slots.size();
		slots.resize(idx + 1);
		new (slots[idx].storage) tracy::VkCtxScope(static_cast<TracyVkCtx>(p_ctx), p_line, p_file, strlen(p_file), p_function, strlen(p_function), name, strlen(name), p_cmd_buf, true);
	}

	_FORCE_INLINE_ void end() {
		if (slots.is_empty()) {
			return;
		}
		const uint32_t idx = slots.size() - 1;
		reinterpret_cast<tracy::VkCtxScope *>(slots[idx].storage)->~VkCtxScope();
		slots.resize(idx);
	}

	_FORCE_INLINE_ void end_all() {
		while (!slots.is_empty()) {
			end();
		}
	}
};

struct GodotTracyVkState {
	Mutex mutex;
	TracyVkCtx ctx = nullptr;
	HashMap<VkCommandBuffer, GodotTracyVkZoneStack *> cmd_stacks;

	static GodotTracyVkState &singleton() {
		static GodotTracyVkState state;
		return state;
	}
};

_FORCE_INLINE_ void GodotProfileVkInit(VkInstance p_instance, VkPhysicalDevice p_physdev, VkDevice p_device, VkQueue p_queue, uint32_t p_queue_family_index, const VkAllocationCallbacks *p_alloc_callbacks, bool p_calibrated) {
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (state.ctx != nullptr) {
		return;
	}

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = p_queue_family_index;

	VkCommandPool temp_pool = VK_NULL_HANDLE;
	if (vkCreateCommandPool(p_device, &pool_info, p_alloc_callbacks, &temp_pool) != VK_SUCCESS) {
		return;
	}

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = temp_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer temp_cmd_buf = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(p_device, &alloc_info, &temp_cmd_buf) == VK_SUCCESS) {
		state.ctx = tracy::CreateVkContext(p_instance, p_physdev, p_device, p_queue, temp_cmd_buf, vkGetInstanceProcAddr, vkGetDeviceProcAddr, p_calibrated);
		if (state.ctx) {
			TracyVkContextName(state.ctx, "Vulkan", 6);
		}
	}

	vkDestroyCommandPool(p_device, temp_pool, p_alloc_callbacks);
}

_FORCE_INLINE_ void GodotProfileVkDestroy() {
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	for (KeyValue<VkCommandBuffer, GodotTracyVkZoneStack *> &E : state.cmd_stacks) {
		memdelete(E.value);
	}
	state.cmd_stacks.clear();
	if (state.ctx) {
		TracyVkDestroy(state.ctx);
		state.ctx = nullptr;
	}
}

_FORCE_INLINE_ void GodotProfileVkCmdBufferBegin(VkCommandBuffer p_cmd_buf, bool p_is_primary) {
	if (p_cmd_buf == VK_NULL_HANDLE) {
		return;
	}
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (!state.ctx) {
		return;
	}
	GodotTracyVkZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_buf);
	GodotTracyVkZoneStack *stack = stack_ptr ? *stack_ptr : nullptr;
	if (!stack) {
		stack = memnew(GodotTracyVkZoneStack);
		state.cmd_stacks.insert(p_cmd_buf, stack);
	}
	stack->is_primary = p_is_primary;
}

_FORCE_INLINE_ void GodotProfileVkZoneBeginImpl(VkCommandBuffer p_cmd_buf, const char *p_name, const char *p_file, const char *p_function, uint32_t p_line) {
	if (p_cmd_buf == VK_NULL_HANDLE) {
		return;
	}
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (!state.ctx) {
		return;
	}
	GodotTracyVkZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_buf);
	GodotTracyVkZoneStack *stack = stack_ptr ? *stack_ptr : nullptr;
	if (!stack) {
		stack = memnew(GodotTracyVkZoneStack);
		state.cmd_stacks.insert(p_cmd_buf, stack);
	}
	stack->begin(state.ctx, p_cmd_buf, p_name, p_file, p_function, p_line);
}

#define GodotProfileVkZoneBegin(m_cmd_buf, m_name) \
	GodotProfileVkZoneBeginImpl(m_cmd_buf, m_name, TracyFile, TracyFunction, (uint32_t)TracyLine)

_FORCE_INLINE_ void GodotProfileVkZoneEnd(VkCommandBuffer p_cmd_buf) {
	if (p_cmd_buf == VK_NULL_HANDLE) {
		return;
	}
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyVkZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_buf)) {
		(*stack_ptr)->end();
	}
}

_FORCE_INLINE_ void GodotProfileVkCmdBufferEnd(VkCommandBuffer p_cmd_buf, bool p_outside_render_pass) {
	if (p_cmd_buf == VK_NULL_HANDLE) {
		return;
	}
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyVkZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_buf)) {
		(*stack_ptr)->end_all();
		if (state.ctx && (*stack_ptr)->is_primary && p_outside_render_pass) {
			TracyVkCollect(state.ctx, p_cmd_buf);
		}
	}
}

_FORCE_INLINE_ void GodotProfileVkFreeCmdBuffer(VkCommandBuffer p_cmd_buf) {
	if (p_cmd_buf == VK_NULL_HANDLE) {
		return;
	}
	GodotTracyVkState &state = GodotTracyVkState::singleton();
	MutexLock lock(state.mutex);
	if (GodotTracyVkZoneStack **stack_ptr = state.cmd_stacks.getptr(p_cmd_buf)) {
		memdelete(*stack_ptr);
		state.cmd_stacks.erase(p_cmd_buf);
	}
}
#endif // GODOT_PROFILING_VULKAN

#elif defined(GODOT_USE_PERFETTO)
// Use the perfetto profiler.

#include "core/typedefs.h"
#include "main/performance.h"

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
		perfetto::Category("godot")
				.SetDescription("Godot Engine Events"),
		perfetto::Category("godot_scripting")
				.SetDescription("Godot Scripting Events"), );

// See PERFETTO_INTERNAL_SCOPED_EVENT_FINALIZER
struct PerfettoGroupedEventEnder {
	_FORCE_INLINE_ void _end_now() {
		TRACE_EVENT_END("godot");
	}

	_FORCE_INLINE_ ~PerfettoGroupedEventEnder() {
		_end_now();
	}
};

#define GodotProfileFrameMark \
	perfetto::CounterTrack __frame_time_track = perfetto::CounterTrack("Frame time", "ms").set_unit_multiplier(1000); \
	TRACE_COUNTER("godot", __frame_time_track, Performance::get_singleton()->get_monitor(Performance::Monitor::TIME_PROCESS));
#define GodotProfileZone(m_zone_name) TRACE_EVENT("godot", m_zone_name);
#define GodotProfileZoneNamed(m_var_name, m_zone_name) GodotProfileZone(m_zone_name)
#define GodotProfileZoneName(m_var_name, m_txt, m_size)
#define GodotProfileZoneGroupedFirst(m_group_name, m_zone_name) \
	TRACE_EVENT_BEGIN("godot", m_zone_name); \
	PerfettoGroupedEventEnder __godot_perfetto_zone_##m_group_name
#define GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name) __godot_perfetto_zone_##m_group_name.~PerfettoGroupedEventEnder()
#define GodotProfileZoneGrouped(m_group_name, m_zone_name) \
	__godot_perfetto_zone_##m_group_name._end_now(); \
	TRACE_EVENT_BEGIN("godot", m_zone_name);

static HashSet<StringName> __tracing_system_call;

/**
 * Script tracing may cross function boundaries (tracing started in the caller script), so the logic below only triggers
 * a TRACE_EVENT_BEGIN for `GodotProfileZoneScript` if tracing hasn't already been initiated by
 * `GodotProfileZoneScriptSystemCall` in the caller.
 */
struct PerfettoScriptTracer {
	StringName name;
	bool is_system_call;
	bool tracing;

	PerfettoScriptTracer(const StringName &p_file, const StringName &p_function, const StringName &p_name, int p_line, bool p_system_call) : name(p_name), is_system_call(p_system_call) {
		if (is_system_call || !__tracing_system_call.erase(name)) {
			TRACE_EVENT_BEGIN("godot_scripting", perfetto::DynamicString(p_name.string().utf8().get_data()), "source file", p_file.string().utf8().get_data(), "line number", p_line);
			tracing = true;
		}

		if (is_system_call) {
			__tracing_system_call.insert(name);
		}
	}

	_FORCE_INLINE_ void _end_now() {
		if (tracing) {
			TRACE_EVENT_END("godot_scripting");
		}
	}

	_FORCE_INLINE_ ~PerfettoScriptTracer() {
		_end_now();
		if (is_system_call) {
			__tracing_system_call.erase(name);
		}
	}
};

#define GodotProfileZoneScript(m_ptr, m_file, m_function, m_name, m_line) PerfettoScriptTracer __godot_perfetto_script_tracer(m_file, m_function, m_name, m_line, false);
#define GodotProfileZoneScriptSystemCall(m_ptr, m_file, m_function, m_name, m_line) PerfettoScriptTracer __godot_perfetto_script_system_call_tracer(m_file, m_function, m_name, m_line, true);

#define GodotProfileAlloc(m_ptr, m_size)
#define GodotProfileFree(m_ptr)

void godot_init_profiler();
void godot_cleanup_profiler();

#elif defined(GODOT_USE_INSTRUMENTS)

#include <os/log.h>
#include <os/signpost.h>

namespace apple::instruments {

extern os_log_t LOG;
extern os_log_t LOG_TRACING;

typedef void (*DeferFunc)();

class Defer {
public:
	explicit Defer(DeferFunc p_fn) :
			_fn(p_fn) {}
	~Defer() {
		_fn();
	}

private:
	DeferFunc _fn;
};

} // namespace apple::instruments

#define GodotProfileFrameMark \
	os_signpost_event_emit(apple::instruments::LOG, OS_SIGNPOST_ID_EXCLUSIVE, "Frame");

#define GodotProfileZoneGroupedFirst(m_group_name, m_zone_name) \
	os_signpost_interval_begin(apple::instruments::LOG_TRACING, OS_SIGNPOST_ID_EXCLUSIVE, m_zone_name); \
	apple::instruments::DeferFunc _GD_VARNAME_CONCAT_(defer__fn, _, m_group_name) = []() { \
		os_signpost_interval_end(apple::instruments::LOG_TRACING, OS_SIGNPOST_ID_EXCLUSIVE, m_zone_name); \
	}; \
	apple::instruments::Defer _GD_VARNAME_CONCAT_(__instruments_defer_zone_end__, _, m_group_name)(_GD_VARNAME_CONCAT_(defer__fn, _, m_group_name));

#define GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name) \
	_GD_VARNAME_CONCAT_(__instruments_defer_zone_end__, _, m_group_name).~Defer();

#define GodotProfileZoneGrouped(m_group_name, m_zone_name) \
	GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name); \
	os_signpost_interval_begin(apple::instruments::LOG_TRACING, OS_SIGNPOST_ID_EXCLUSIVE, m_zone_name); \
	_GD_VARNAME_CONCAT_(defer__fn, _, m_group_name) = []() { \
		os_signpost_interval_end(apple::instruments::LOG_TRACING, OS_SIGNPOST_ID_EXCLUSIVE, m_zone_name); \
	}; \
	new (&_GD_VARNAME_CONCAT_(__instruments_defer_zone_end__, _, m_group_name)) apple::instruments::Defer(_GD_VARNAME_CONCAT_(defer__fn, _, m_group_name));

#define GodotProfileZone(m_zone_name) \
	GodotProfileZoneGroupedFirst(__COUNTER__, m_zone_name)
#define GodotProfileZoneNamed(m_var_name, m_zone_name) GodotProfileZone(m_zone_name)
#define GodotProfileZoneName(m_var_name, m_txt, m_size)

#define GodotProfileZoneScript(m_ptr, m_file, m_function, m_name, m_line)
#define GodotProfileZoneScriptSystemCall(m_ptr, m_file, m_function, m_name, m_line)

// Instruments has its own memory profiling, so these are no-ops.
#define GodotProfileAlloc(m_ptr, m_size)
#define GodotProfileFree(m_ptr)

void godot_init_profiler();
void godot_cleanup_profiler();

#else
// No profiling; all macros are stubs.

void godot_init_profiler();
void godot_cleanup_profiler();

// Tell the profiling backend that a new frame has started.
#define GodotProfileFrameMark
// Defines a profile zone from here to the end of the scope.
#define GodotProfileZone(m_zone_name)
#define GodotProfileZoneNamed(m_var_name, m_zone_name)
#define GodotProfileZoneName(m_var_name, m_txt, m_size)
// Defines a profile zone group. The first profile zone starts immediately,
// and ends either when the next zone starts, or when the scope ends.
#define GodotProfileZoneGroupedFirst(m_group_name, m_zone_name)
// End the profile zone group's current profile zone now.
#define GodotProfileZoneGroupedEndEarly(m_group_name, m_zone_name)
// Replace the profile zone group's current profile zone.
// The new zone ends either when the next zone starts, or when the scope ends.
#define GodotProfileZoneGrouped(m_group_name, m_zone_name)
// Tell the profiling backend that an allocation happened, with its location and size.
#define GodotProfileAlloc(m_ptr, m_size)
// Tell the profiling backend that an allocation was freed.
// There must be a one to one correspondence of GodotProfileAlloc and GodotProfileFree calls.
#define GodotProfileFree(m_ptr)

// Define a zone for a script call (dynamic source location).
// m_ptr is a pointer to the function instance, which will be used for the lookup.
// m_file, m_function, m_name are StringNames, and m_line is uint32_t
#define GodotProfileZoneScript(m_ptr, m_file, m_function, m_name, m_line)
// Define a zone for a system call from a script (dynamic source location).
#define GodotProfileZoneScriptSystemCall(m_ptr, m_file, m_function, m_name, m_line)

// Add a scoped block with the name of the function
#define GodotProfileFunction()
// Start a profiling block with a non-const name
#define GodotProfileDynamicZoneStart(...)
// End the current dynamic profiling block
#define GodotProfileDynamicZoneEnd()
// Adds a name for the current thread in the profiler
#define GodotProfilingThreadStart(NAME)
// Sends a screenshot (if the profiler supports it)
#define GodotProfileSendScreenshot()
// Returns whether GPU profiling is active.
#define GodotProfileHasGPU() false

#endif

#if !defined(GODOT_USE_TRACY)

#if defined(GODOT_USE_PERFETTO) || defined(GODOT_USE_INSTRUMENTS)
#define GodotProfileHasGPU() false
#endif

#ifdef GODOT_PROFILING_D3D12
#define GodotProfileD3D12Init(p_device, p_queue)
#define GodotProfileD3D12Destroy()
#define GodotProfileD3D12NewFrame(p_queue)
#define GodotProfileD3D12Collect()
#define GodotProfileD3D12ZoneBegin(m_cmd_list, m_name)
#define GodotProfileD3D12ZoneEnd(p_cmd_list)
#define GodotProfileD3D12ZoneEndAll(p_cmd_list)
#define GodotProfileD3D12FreeCmdList(p_cmd_list)
#endif

#ifdef GODOT_PROFILING_VULKAN
#define GodotProfileVkInit(p_instance, p_physdev, p_device, p_queue, p_queue_family_index, p_alloc_callbacks, p_calibrated)
#define GodotProfileVkDestroy()
#define GodotProfileVkCmdBufferBegin(p_cmd_buf, p_is_primary)
#define GodotProfileVkZoneBegin(m_cmd_buf, m_name)
#define GodotProfileVkZoneEnd(p_cmd_buf)
#define GodotProfileVkCmdBufferEnd(p_cmd_buf, p_outside_render_pass)
#define GodotProfileVkFreeCmdBuffer(p_cmd_buf)
#endif

#endif