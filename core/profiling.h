#ifndef GODOT_SOURCE_PROFILING_H
#define GODOT_SOURCE_PROFILING_H

#ifdef PROFILING_ENABLED
#include "tracy/Tracy.hpp"
#include "common/TracySystem.hpp"
#include <core/os/thread.h>

#define PROFILE_FRAME(NAME) FrameMark;
#define PROFILE_FUNCTION(...) ZoneScoped;
#define PROFILE_DYNAMIC_FUNCTION(NAME) ZoneScoped; ZoneName(NAME, strlen(NAME)); ZoneColor(0x008000);
#define PROFILING_THREAD(NAME)
#define PROFILING_GPU_INIT_VULKAN(DEVICES, PHYSICAL_DEVICES, CMD_QUEUES, CMD_QUEUES_FAMILY, NUM_CMD_QUEUES, FUNCTIONS)
#define PROFILING_GPU_FLIP(SWAP_CHAIN)
#define PROFILING_SEND_SCREENSHOT() \
    if(TracyIsConnected) { \
        ZoneScopedN("Profiling Screenshot"); ZoneColor(0x02055A); \
        RID main_vp_rid = RenderingServer::get_singleton()->viewport_find_from_screen_attachment(DisplayServer::MAIN_WINDOW_ID); \
        RID main_vp_texture = RenderingServer::get_singleton()->viewport_get_texture(main_vp_rid); \
        Ref<Image> vp_tex = RenderingServer::get_singleton()->texture_2d_get(main_vp_texture); \
        vp_tex->convert(Image::FORMAT_RGBA8); \
        FrameImage(vp_tex->get_data().ptr(), vp_tex->get_width(), vp_tex->get_height(), 0, false); \
    };
#define PROFILING_ALLOC(PTR , SIZE) TracyAlloc(PTR , SIZE);
#define PROFILING_FREE(PTR) TracyFree(PTR);
#define PROFILING_START() system("start cmd /c capture.exe -o output.tracy -f -s 5"); print_line("Started tracy capture");
#else
#define PROFILE_FRAME(NAME)
#define PROFILE_FUNCTION(...)
#define PROFILE_DYNAMIC_FUNCTION(...)
#define PROFILING_THREAD(NAME)
#define PROFILING_GPU_INIT_VULKAN(...)
#define PROFILING_GPU_FLIP(SWAP_CHAIN)
#define PROFILING_SEND_SCREENSHOT()
#define PROFILING_ALLOC(...)
#define PROFILING_FREE(...)
#define PROFILING_START()
#endif

#endif //GODOT_SOURCE_PROFILING_H
