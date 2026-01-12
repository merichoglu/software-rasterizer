#include "pipeline/shadow_map.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

ShadowMap::ShadowMap() :
    width(0),
    height(0),
    light_view_matrix(1.0f),
    light_projection_matrix(1.0f),
    light_space_matrix(1.0f),
    bias(0.005f)
{}

ShadowMap::ShadowMap(int i_width, int i_height) :
    width(i_width),
    height(i_height),
    light_view_matrix(1.0f),
    light_projection_matrix(1.0f),
    light_space_matrix(1.0f),
    bias(0.005f)
{
    depth_buffer.resize(width * height, 1.0f);
}

void ShadowMap::resize(int i_width, int i_height) {
    width = i_width;
    height = i_height;
    depth_buffer.resize(width * height, 1.0f);
}

void ShadowMap::clear() {
    std::fill(depth_buffer.begin(), depth_buffer.end(), 1.0f);
}

void ShadowMap::set_depth(int x, int y, float depth) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        depth_buffer[y * width + x] = depth;
    }
}

float ShadowMap::get_depth(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return depth_buffer[y * width + x];
    }
    return 1.0f;
}

bool ShadowMap::depth_test(int x, int y, float new_depth) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }

    int idx = y * width + x;
    if (new_depth < depth_buffer[idx]) {
        depth_buffer[idx] = new_depth;
        return true;
    }
    return false;
}

void ShadowMap::setup_directional_light(Vec3 direction, Vec3 scene_center, float scene_radius) {
    /* normalize light direction */
    Vec3 light_dir = glm::normalize(direction);

    /* position the light far enough to cover the scene */
    Vec3 light_pos = scene_center - light_dir * scene_radius * 2.0f;

    /* create view matrix looking at scene center */
    light_view_matrix = glm::lookAt(light_pos, scene_center, Vec3(0.0f, 1.0f, 0.0f));

    /* orthographic projection to cover the scene */
    float ortho_size = scene_radius * 1.5f;
    light_projection_matrix = glm::ortho(
        -ortho_size, ortho_size,
        -ortho_size, ortho_size,
        0.1f, scene_radius * 4.0f
    );

    /* combined light space matrix */
    light_space_matrix = light_projection_matrix * light_view_matrix;
}

Vec3 ShadowMap::world_to_shadow_uv(Vec3 world_pos) const {
    /* transform to light clip space */
    Vec4 light_clip = light_space_matrix * Vec4(world_pos, 1.0f);

    /* perspective divide (though ortho, w should be 1) */
    Vec3 ndc = Vec3(light_clip) / light_clip.w;

    /* convert from [-1,1] to [0,1] */
    Vec3 shadow_uv;
    shadow_uv.x = ndc.x * 0.5f + 0.5f;
    shadow_uv.y = ndc.y * 0.5f + 0.5f;
    shadow_uv.z = ndc.z * 0.5f + 0.5f;  /* depth in [0,1] */

    return shadow_uv;
}

bool ShadowMap::is_in_shadow(Vec3 world_pos) const {
    Vec3 shadow_uv = world_to_shadow_uv(world_pos);

    /* check if outside shadow map */
    if (shadow_uv.x < 0.0f || shadow_uv.x > 1.0f ||
        shadow_uv.y < 0.0f || shadow_uv.y > 1.0f) {
        return false;
    }

    /* get shadow map coordinates */
    int x = static_cast<int>(shadow_uv.x * (width - 1));
    int y = static_cast<int>((1.0f - shadow_uv.y) * (height - 1));  /* flip Y */

    float stored_depth = get_depth(x, y);
    float current_depth = shadow_uv.z;

    /* in shadow if current depth > stored depth + bias */
    return current_depth > stored_depth + bias;
}

float ShadowMap::sample_shadow_pcf(Vec3 world_pos, int kernel_size) const {
    Vec3 shadow_uv = world_to_shadow_uv(world_pos);

    /* check if outside shadow map */
    if (shadow_uv.x < 0.0f || shadow_uv.x > 1.0f ||
        shadow_uv.y < 0.0f || shadow_uv.y > 1.0f) {
        return 0.0f;  /* not in shadow */
    }

    float current_depth = shadow_uv.z;
    int center_x = static_cast<int>(shadow_uv.x * (width - 1));
    int center_y = static_cast<int>((1.0f - shadow_uv.y) * (height - 1));

    /* PCF: sample surrounding texels and average */
    int half_kernel = kernel_size / 2;
    float shadow = 0.0f;
    int samples = 0;

    for (int dy = -half_kernel; dy <= half_kernel; ++dy) {
        for (int dx = -half_kernel; dx <= half_kernel; ++dx) {
            int sx = center_x + dx;
            int sy = center_y + dy;

            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                float stored_depth = get_depth(sx, sy);
                if (current_depth > stored_depth + bias) {
                    shadow += 1.0f;
                }
                samples++;
            }
        }
    }

    return (samples > 0) ? shadow / samples : 0.0f;
}

int ShadowMap::get_width() const {
    return width;
}

int ShadowMap::get_height() const {
    return height;
}

Mat4 ShadowMap::get_light_space_matrix() const {
    return light_space_matrix;
}

void ShadowMap::set_bias(float i_bias) {
    bias = i_bias;
}
