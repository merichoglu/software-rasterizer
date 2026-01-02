#pragma once

#include "math/vector.h"
#include "pipeline/rasterizer.h"
#include <vector>

/* light types */
enum class LightType {
    DIRECTIONAL,    /* sun-like, parallel rays */
    POINT,          /* radiates in all directions */
    SPOT            /* cone of light */
};

/* light source definition */
struct Light {
    LightType type;
    Vec3 position;          /* for point/spot lights */
    Vec3 direction;         /* for directional/spot lights */
    Color color;            /* light color */
    float intensity;        /* light strength */

    /* attenuation for point/spot lights */
    float constant_atten;
    float linear_atten;
    float quadratic_atten;

    /* spot light cone */
    float inner_cutoff;     /* cosine of inner angle */
    float outer_cutoff;     /* cosine of outer angle */

    Light() :
        type(LightType::DIRECTIONAL),
        position(0.0f),
        direction(0.0f, -1.0f, 0.0f),
        color(1.0f),
        intensity(1.0f),
        constant_atten(1.0f),
        linear_atten(0.09f),
        quadratic_atten(0.032f),
        inner_cutoff(0.9763f),  /* cos(12.5 degrees) */
        outer_cutoff(0.9659f)   /* cos(15 degrees) */
    {}
};

/* material properties */
struct Material {
    Color ambient;          /* ambient reflectance */
    Color diffuse;          /* diffuse reflectance */
    Color specular;         /* specular reflectance */
    float shininess;        /* specular exponent */

    Material() :
        ambient(0.2f, 0.2f, 0.2f, 1.0f),
        diffuse(0.8f, 0.8f, 0.8f, 1.0f),
        specular(1.0f, 1.0f, 1.0f, 1.0f),
        shininess(32.0f)
    {}
};

class FragmentProcessor {
    private:
        std::vector<Light> lights;
        Material material;
        Color ambient_light;
        Vec3 camera_position;

        /* calculate contribution from a single light */
        Color calculate_light(const Light& light, Vec3 world_pos, Vec3 normal, Vec3 view_dir);

        /* attenuation calculation for point/spot lights */
        float calculate_attenuation(const Light& light, float distance);

    public:
        /* constructor */
        FragmentProcessor();

        /* light management */
        void add_light(const Light& light);
        void clear_lights();
        void set_ambient_light(Color color);

        /* material management */
        void set_material(const Material& i_material);

        /* set camera position for specular calculations */
        void set_camera_position(Vec3 position);

        /* process a fragment and return final color (fragment shader) */
        Color process_fragment(const Fragment& fragment);

        /* shading modes */
        Color shade_flat(const Fragment& fragment);
        Color shade_phong(const Fragment& fragment);
};
