#include "framebuffer.h"
#include <algorithm>

FrameBuffer::FrameBuffer(int i_width, int i_height) {
    width = i_width;
    height = i_height;
    color_buffer.resize(width * height);
    depth_buffer.resize(width * height);
    clear(Colors::black());
    clear_depth(1.0f);
}

void FrameBuffer::clear(Color color) {
    for (int i = 0; i < width * height; i++) {
        color_buffer[i] = color;
    }
}

void FrameBuffer::clear_depth(float value) {
    for (int i = 0; i < width * height; i++) {
        depth_buffer[i] = value;
    }
}

void FrameBuffer::set_pixel(int x, int y, Color color) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    int index = y * width + x;
    color_buffer[index] = color;
}

void FrameBuffer::set_pixel_blended(int x, int y, Color color, BlendMode mode) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    int index = y * width + x;

    if (mode == BlendMode::NONE) {
        color_buffer[index] = color;
        return;
    }

    Color dst = color_buffer[index];
    Color result;

    switch (mode) {
        case BlendMode::ALPHA: {
            /* standard alpha blending: src * alpha + dst * (1 - alpha) */
            float alpha = color.a;
            float inv_alpha = 1.0f - alpha;
            result.r = color.r * alpha + dst.r * inv_alpha;
            result.g = color.g * alpha + dst.g * inv_alpha;
            result.b = color.b * alpha + dst.b * inv_alpha;
            result.a = alpha + dst.a * inv_alpha;
            break;
        }
        case BlendMode::ADDITIVE: {
            /* additive blending: src + dst */
            result.r = std::min(color.r + dst.r, 1.0f);
            result.g = std::min(color.g + dst.g, 1.0f);
            result.b = std::min(color.b + dst.b, 1.0f);
            result.a = std::min(color.a + dst.a, 1.0f);
            break;
        }
        case BlendMode::MULTIPLY: {
            /* multiply blending: src * dst */
            result.r = color.r * dst.r;
            result.g = color.g * dst.g;
            result.b = color.b * dst.b;
            result.a = color.a * dst.a;
            break;
        }
        default:
            result = color;
            break;
    }

    color_buffer[index] = result;
}

Color FrameBuffer::get_pixel(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return Colors::black();
    }
    int index = y * width + x;
    return color_buffer[index];
}

void FrameBuffer::set_depth(int x, int y, float depth) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    int index = y * width + x;
    depth_buffer[index] = depth;
}

float FrameBuffer::get_depth(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 1.0f;
    }
    int index = y * width + x;
    return depth_buffer[index];
}

bool FrameBuffer::depth_test(int x, int y, float new_depth) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    int index = y * width + x;
    if (new_depth < depth_buffer[index]) {
        depth_buffer[index] = new_depth;
        return true;
    }
    return false;
}

int FrameBuffer::get_width() {
    return width;
}

int FrameBuffer::get_height() {
    return height;
}

std::vector<Color>& FrameBuffer::get_color_buffer() {
    return color_buffer;
}

std::vector<float>& FrameBuffer::get_depth_buffer() {
    return depth_buffer;
}
