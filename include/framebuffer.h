#pragma once

#include "math/vector.h"
#include <vector>

class FrameBuffer {
    private:
        int width;
        int height;
        std::vector<Color> color_buffer;
        std::vector<float> depth_buffer;

    public:
        /* constructor */
        FrameBuffer(int i_width, int i_height);

        /* clear color buffer to specified color */
        void clear(Color color);

        /* clear depth buffer to specified value (default 1.0 = far) */
        void clear_depth(float value = 1.0f);

        /* set pixel color at (x, y) */
        void set_pixel(int x, int y, Color color);

        /* get pixel color at (x, y) */
        Color get_pixel(int x, int y);

        /* set depth at (x, y) */
        void set_depth(int x, int y, float depth);

        /* get depth at (x, y) */
        float get_depth(int x, int y);

        /* depth test: returns true if new_depth is closer, also updates buffer */
        bool depth_test(int x, int y, float new_depth);

        /* getters */
        int get_width();
        int get_height();
        std::vector<Color>& get_color_buffer();
        std::vector<float>& get_depth_buffer();
};