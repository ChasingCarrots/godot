/**************************************************************************/
/*  shader_rd_lazy_variants.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                        */
/**************************************************************************/

#include "shader_rd.h"

#include "core/profiling/loading_trace.h"
#include "servers/rendering/rendering_device.h"

// A cache hit brings in the bytecode for every variant of a group, but a run only ever
// draws a handful of them. Turning bytecode into an RD shader is the expensive part
// (it serializes on RenderingDevice's global mutex), so it is deferred until a variant
// is actually asked for. Variants produced by a real compile already hold their RID and
// fall straight through.
RID ShaderRD::_materialize_variant(Version *p_version, int p_variant) {
	if (p_variant >= p_version->variant_pending.size() || p_version->variant_pending[p_variant] == 0) {
		return p_version->variants[p_variant];
	}

	LoadingTraceSpan _lt(LT_SH_CACHE_LOAD, name, "materialize");
	_lt.args((uint32_t)p_variant);

	// Pass the current RID as the placeholder so an already handed-out placeholder is
	// upgraded in place, exactly as the eager cache path used to do.
	RID shader = RD::get_singleton()->shader_create_from_bytecode_with_samplers(
			p_version->variant_data[p_variant], p_version->variants[p_variant], immutable_samplers);
	ERR_FAIL_COND_V_MSG(shader.is_null(), RID(),
			vformat("Failed to create shader '%s' variant %d from cached bytecode.", name, p_variant));

	p_version->variants.write[p_variant] = shader;
	p_version->variant_pending.write[p_variant] = 0;
	return shader;
}
