/**************************************************************************/
/*  render_forward_clustered_pso.h                                        */
/**************************************************************************/

#pragma once

#include "scene_shader_forward_clustered.h"

namespace RendererSceneRenderImplementation {

// Record/replay of the pipelines this renderer actually draws with. See pso_record.h for why a
// pipeline is stored as the inputs that produce its key rather than as the key itself.
class RenderForwardClusteredPSO {
public:
	static void record(SceneShaderForwardClustered::ShaderData *p_shader,
			const SceneShaderForwardClustered::ShaderData::PipelineKey &p_key, uint64_t p_surface_format,
			bool p_instanced, bool p_motion_vectors, bool p_point_size);

	static void init();
};

} // namespace RendererSceneRenderImplementation
