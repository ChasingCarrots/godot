// Depth-only render path for viewports created with `viewport_set_depth_only`.
//
// This lives in its own translation unit on purpose. `render_forward_clustered.cpp`
// changes constantly upstream, so keeping this pass out of it means merges only ever
// have to deal with the single declaration added to `render_forward_clustered.h`.

#include "render_forward_clustered.h"

#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_globals.h"

using namespace RendererSceneRenderImplementation;

bool RenderForwardClustered::_render_depth_only(RenderDataRD *p_render_data, const Color &p_default_bg_color) {
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND_V(rb.is_null(), false);

	if (!rb->has_custom_data(RB_SCOPE_FORWARD_CLUSTERED)) {
		// No forward clustered buffers means no depth framebuffer to render into.
		return false;
	}
	Ref<RenderBufferDataForwardClustered> rb_data = rb->get_custom_data(RB_SCOPE_FORWARD_CLUSTERED);
	ERR_FAIL_COND_V(rb_data.is_null(), false);

	if (rb->get_msaa_3d() != RSE::VIEWPORT_MSAA_DISABLED) {
		// Resolving MSAA depth would undo most of what this path is here to save, and a
		// compositor effect can only sample the resolved texture. Fall back to a regular
		// render rather than silently producing a black viewport.
		WARN_PRINT_ONCE("Depth-only viewports do not support MSAA. Set the viewport's MSAA 3D to disabled.");
		return false;
	}

	RENDER_TIMESTAMP("Setup Depth-Only Render");

	RD::get_singleton()->draw_command_begin_label("Render Depth Only");

	RenderSceneDataRD *scene_data = p_render_data->scene_data;
	const Size2i screen_size = rb->get_internal_size();
	const bool reverse_cull = scene_data->cam_transform.basis.determinant() < 0;

	scene_data->emissive_exposure_normalization = -1.0;

	// No light clustering happens in this pass, but `_setup_environment` still divides the
	// screen size by the cluster size, so it must not be left at its default of 0. Same
	// values the other non-clustered passes (shadow, material, UV2, SDFGI) use.
	p_render_data->cluster_size = 1;
	p_render_data->cluster_max_elements = 32;

	// Clears the stale lightmap/voxel GI counts left behind by whichever viewport
	// rendered before this one. Both lists were emptied by `render_scene`.
	_setup_lightmaps(p_render_data, *p_render_data->lightmaps, scene_data->cam_transform);
	_setup_voxelgis(*p_render_data->voxel_gi_instances);

	const uint32_t uniform_buffer_index = _setup_environment(p_render_data, false, screen_size, screen_size, p_default_bg_color, false);

	// May have changed due to the above.
	_update_render_base_uniform_set();

	// `PASS_MODE_DEPTH` collects the same surfaces as the regular depth pre-pass, so
	// alpha-tested geometry still cuts correctly. No lights, decals, GI or sky are set
	// up for this pass; `render_scene` has already emptied those lists.
	_fill_render_list(RENDER_LIST_OPAQUE, p_render_data, PASS_MODE_DEPTH);
	render_list[RENDER_LIST_OPAQUE].sort_by_key();

	int *render_info = p_render_data->render_info ? p_render_data->render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_VISIBLE] : (int *)nullptr;
	_fill_instance_data(RENDER_LIST_OPAQUE, render_info);

	RID rp_uniform_set = _setup_render_pass_uniform_set(RENDER_LIST_OPAQUE, nullptr, RID(), rb->get_samplers(), uniform_buffer_index);

	RENDER_TIMESTAMP("Render Depth Only");

	{
		RID depth_framebuffer = rb_data->get_depth_fb();
		RenderListParameters render_list_params(
				render_list[RENDER_LIST_OPAQUE].elements.ptr(),
				render_list[RENDER_LIST_OPAQUE].element_info.ptr(),
				render_list[RENDER_LIST_OPAQUE].elements.size(),
				reverse_cull,
				PASS_MODE_DEPTH,
				0,
				true, // no_gi
				false, // use_directional_soft_shadows
				rp_uniform_set,
				get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_WIREFRAME,
				Vector2(),
				scene_data->lod_distance_multiplier,
				scene_data->screen_mesh_lod_threshold,
				scene_data->view_count,
				0,
				scene_shader.default_specialization);

		_render_list_with_draw_list(&render_list_params, depth_framebuffer, RD::DRAW_CLEAR_ALL, Vector<Color>(), 0.0f, 0u, p_render_data->render_region);
	}

	RD::get_singleton()->draw_command_end_label();

	// Nothing in this path writes color, so the color buffer still holds the previous
	// frame. Clear it before handing over, so anything the compositor effect leaves
	// untouched reads as the background rather than as a stale image. Depth is loaded,
	// not cleared, so the effect can still sample what was just rendered.
	{
		RD::get_singleton()->draw_command_begin_label("Clear Depth-Only Color");

		Vector<Color> clear_colors;
		clear_colors.push_back(p_default_bg_color);

		RD::get_singleton()->draw_list_begin(rb_data->get_color_only_fb(), RD::DRAW_CLEAR_COLOR_ALL, clear_colors);
		RD::get_singleton()->draw_list_end();

		RD::get_singleton()->draw_command_end_label();
	}

	RENDER_TIMESTAMP("Process Post Opaque Compositor Effects");
	_process_compositor_effects(RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_OPAQUE, p_render_data);

	// Also consumes the render target's clear request, as in the regular render path.
	RENDER_TIMESTAMP("Tonemap");
	_render_buffers_post_process_and_tonemap(p_render_data);

	return true;
}
