#include "pipeline/fragment_processor.h"
#include <algorithm>

FragmentProcessor::FragmentProcessor() {
    ambient_light = Color(0.1f, 0.1f, 0.1f, 1.0f);
    camera_position = Vec3(0.0f, 0.0f, 0.0f);
}

void FragmentProcessor::add_light(const Light& light) {
    lights.push_back(light);
}

void FragmentProcessor::clear_lights() {
    lights.clear();
}

void FragmentProcessor::set_ambient_light(Color color) {
    ambient_light = color;
}

void FragmentProcessor::set_material(const Material& i_material) {
    material = i_material;
}

void FragmentProcessor::set_camera_position(Vec3 position) {
    camera_position = position;
}

Color FragmentProcessor::process_fragment(const Fragment& fragment) {
    return shade_phong(fragment);
}

Color FragmentProcessor::shade_flat(const Fragment& fragment) {
    /* simply return interpolated vertex color */
    return fragment.color;
}

Color FragmentProcessor::shade_phong(const Fragment& fragment) {
    /* per-pixel Blinn-Phong lighting */

    /* get surface normal (interpolated and normalized) */
    Vec3 normal = glm::normalize(fragment.normal);

    /* get base color from vertex color or material */
    Color base_color = fragment.color * material.diffuse;

    /* view direction (from fragment to camera) */
    Vec3 view_dir = glm::normalize(camera_position - fragment.world_pos);

    /* start with ambient contribution */
    Color result = ambient_light * material.ambient * base_color;

    /* add contribution from each light */
    for (const Light& light : lights) {
        Color light_contrib = calculate_light(light, fragment.world_pos, normal, view_dir);
        result = result + light_contrib * base_color;
    }

    /* clamp final color to [0, 1] */
    result.r = std::clamp(result.r, 0.0f, 1.0f);
    result.g = std::clamp(result.g, 0.0f, 1.0f);
    result.b = std::clamp(result.b, 0.0f, 1.0f);
    result.a = base_color.a;

    return result;
}

Color FragmentProcessor::calculate_light(const Light& light, Vec3 world_pos, Vec3 normal, Vec3 view_dir) {
    Vec3 light_dir;
    float attenuation = 1.0f;

    /* calculate light direction and attenuation based on light type */
    switch (light.type) {
        case LightType::DIRECTIONAL: {
            light_dir = glm::normalize(-light.direction);
            attenuation = 1.0f;
            break;
        }
        case LightType::POINT: {
            Vec3 light_vec = light.position - world_pos;
            float distance = glm::length(light_vec);
            light_dir = light_vec / distance;
            attenuation = calculate_attenuation(light, distance);
            break;
        }
        case LightType::SPOT: {
            Vec3 light_vec = light.position - world_pos;
            float distance = glm::length(light_vec);
            light_dir = light_vec / distance;
            attenuation = calculate_attenuation(light, distance);

            /* spot cone attenuation */
            float theta = glm::dot(light_dir, glm::normalize(-light.direction));
            float epsilon = light.inner_cutoff - light.outer_cutoff;
            float spot_intensity = std::clamp((theta - light.outer_cutoff) / epsilon, 0.0f, 1.0f);
            attenuation *= spot_intensity;
            break;
        }
    }

    /* diffuse component (Lambertian) */
    float n_dot_l = std::max(glm::dot(normal, light_dir), 0.0f);
    Color diffuse = material.diffuse * n_dot_l;

    /* specular component (Blinn-Phong) */
    Vec3 halfway_dir = glm::normalize(light_dir + view_dir);
    float n_dot_h = std::max(glm::dot(normal, halfway_dir), 0.0f);
    float spec = std::pow(n_dot_h, material.shininess);
    Color specular = material.specular * spec;

    /* combine */
    Color result = (diffuse + specular) * light.color * light.intensity * attenuation;

    return result;
}

float FragmentProcessor::calculate_attenuation(const Light& light, float distance) {
    /* standard attenuation formula */
    /* attenuation = 1 / (constant + linear * d + quadratic * d^2) */
    return 1.0f / (
        light.constant_atten +
        light.linear_atten * distance +
        light.quadratic_atten * distance * distance
    );
}
