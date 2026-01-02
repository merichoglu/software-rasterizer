#include "output.h"
#include <fstream>
#include <algorithm>

namespace Output {

bool save_to_ppm(FrameBuffer& framebuffer, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    int width = framebuffer.get_width();
    int height = framebuffer.get_height();

    /* PPM header */
    file << "P6\n" << width << " " << height << "\n255\n";

    /* pixel data (RGB, top to bottom) */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color color = framebuffer.get_pixel(x, y);

            /* convert from [0,1] to [0,255] and clamp */
            uint8_t r = static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
            uint8_t b = static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));

            file.write(reinterpret_cast<char*>(&r), 1);
            file.write(reinterpret_cast<char*>(&g), 1);
            file.write(reinterpret_cast<char*>(&b), 1);
        }
    }

    file.close();
    return true;
}

bool save_to_tga(FrameBuffer& framebuffer, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    int width = framebuffer.get_width();
    int height = framebuffer.get_height();

    /* TGA header (18 bytes) */
    uint8_t header[18] = {0};
    header[2] = 2;                              /* uncompressed true-color */
    header[12] = width & 0xFF;                  /* width low byte */
    header[13] = (width >> 8) & 0xFF;           /* width high byte */
    header[14] = height & 0xFF;                 /* height low byte */
    header[15] = (height >> 8) & 0xFF;          /* height high byte */
    header[16] = 32;                            /* bits per pixel (BGRA) */
    header[17] = 0x28;                          /* top-left origin, 8 alpha bits */

    file.write(reinterpret_cast<char*>(header), 18);

    /* pixel data (BGRA format, top to bottom) */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color color = framebuffer.get_pixel(x, y);

            /* TGA uses BGRA order */
            uint8_t b = static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
            uint8_t r = static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
            uint8_t a = static_cast<uint8_t>(std::clamp(color.a * 255.0f, 0.0f, 255.0f));

            file.write(reinterpret_cast<char*>(&b), 1);
            file.write(reinterpret_cast<char*>(&g), 1);
            file.write(reinterpret_cast<char*>(&r), 1);
            file.write(reinterpret_cast<char*>(&a), 1);
        }
    }

    file.close();
    return true;
}

bool save(FrameBuffer& framebuffer, const std::string& filepath) {
    /* find extension */
    size_t dot_pos = filepath.rfind('.');
    if (dot_pos == std::string::npos) {
        return save_to_ppm(framebuffer, filepath + ".ppm");
    }

    std::string ext = filepath.substr(dot_pos + 1);

    /* convert to lowercase */
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "ppm") {
        return save_to_ppm(framebuffer, filepath);
    } else if (ext == "tga") {
        return save_to_tga(framebuffer, filepath);
    }

    /* default to PPM */
    return save_to_ppm(framebuffer, filepath);
}

}
