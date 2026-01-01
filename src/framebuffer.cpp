#include "framebuffer.h"

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
