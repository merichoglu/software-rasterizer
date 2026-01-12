#include "pipeline/fragment_processor.h"
#include <algorithm>

FragmentProcessor::FragmentProcessor() :
    ambient_light(0.1f, 0.1f, 0.1f, 1.0f),
    camera_position(0.0f),
    shadow_map(nullptr),
    shadows_enabled(false)
{}

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

void FragmentProcessor::set_shadow_map(ShadowMap* map) {
    shadow_map = map;
}

void FragmentProcessor::enable_shadows(bool enable) {
    shadows_enabled = enable;
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

    /* get base color: sample diffuse texture if available, otherwise use vertex color * material */
    Color base_color;
    if (material.diffuse_map && material.diffuse_map->is_valid()) {
        base_color = material.diffuse_map->sample(fragment.tex_coord) * fragment.color;
    } else {
        base_color = fragment.color * material.diffuse;
    }

    /* get specular intensity from texture if available */
    Color spec_color = material.specular;
    if (material.specular_map && material.specular_map->is_valid()) {
        spec_color = material.specular_map->sample(fragment.tex_coord);
    }

    /* calculate shadow factor (0 = fully lit, 1 = fully shadowed) */
    float shadow = 0.0f;
    if (shadows_enabled && shadow_map) {
        shadow = shadow_map->sample_shadow_pcf(fragment.world_pos, 3);
    }

    /* view direction (from fragment to camera) */
    Vec3 view_dir = glm::normalize(camera_position - fragment.world_pos);

    /* start with ambient contribution (not affected by shadows) */
    Color result = ambient_light * material.ambient * base_color;

    /* add contribution from each light */
    for (const Light& light : lights) {
        Color light_contrib = calculate_light(light, fragment.world_pos, normal, view_dir, spec_color, shadow);
        result = result + light_contrib * base_color;
    }

    /* clamp final color to [0, 1] */
    result.r = std::clamp(result.r, 0.0f, 1.0f);
    result.g = std::clamp(result.g, 0.0f, 1.0f);
    result.b = std::clamp(result.b, 0.0f, 1.0f);
    /* preserve material alpha for transparency */
    result.a = material.diffuse.a;

    return result;
}

Color FragmentProcessor::calculate_light(const Light& light, Vec3 world_pos, Vec3 normal, Vec3 view_dir, Color spec_color, float shadow) {
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
    Color specular = spec_color * spec;

    /* apply shadow (reduces diffuse and specular, not ambient) */
    float light_factor = 1.0f - shadow;

    /* combine */
    Color result = (diffuse + specular) * light.color * light.intensity * attenuation * light_factor;

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
