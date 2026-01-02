#pragma once

#include "framebuffer.h"
#include <string>

namespace Output {
    /* save framebuffer to PPM file (simple, no dependencies) */
    bool save_to_ppm(FrameBuffer& framebuffer, const std::string& filepath);

    /* save framebuffer to TGA file (supports alpha) */
    bool save_to_tga(FrameBuffer& framebuffer, const std::string& filepath);

    /* generic save - determines format from extension */
    bool save(FrameBuffer& framebuffer, const std::string& filepath);
}
