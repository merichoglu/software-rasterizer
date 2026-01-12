#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include <vector>

class ShadowMap {
    private:
        std::vector<float> depth_buffer;
        int width;
        int height;
        Mat4 light_view_matrix;
        Mat4 light_projection_matrix;
        Mat4 light_space_matrix;
        float bias;

    public:
        ShadowMap();
        ShadowMap(int i_width, int i_height);

        /* resize shadow map */
        void resize(int i_width, int i_height);

        /* clear depth buffer */
        void clear();

        /* set depth at pixel */
        void set_depth(int x, int y, float depth);

        /* get depth at pixel */
        float get_depth(int x, int y) const;

        /* depth test - returns true if new depth is closer */
        bool depth_test(int x, int y, float new_depth);

        /* setup light matrices for directional light */
        void setup_directional_light(Vec3 direction, Vec3 scene_center, float scene_radius);

        /* transform world position to shadow map UV + depth */
        Vec3 world_to_shadow_uv(Vec3 world_pos) const;

        /* check if a world position is in shadow */
        bool is_in_shadow(Vec3 world_pos) const;

        /* sample shadow with PCF (percentage closer filtering) */
        float sample_shadow_pcf(Vec3 world_pos, int kernel_size = 3) const;

        /* getters */
        int get_width() const;
        int get_height() const;
        Mat4 get_light_space_matrix() const;
        void set_bias(float i_bias);
};
