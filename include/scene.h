#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "model_loader.h"
#include "pipeline/fragment_processor.h"
#include <vector>
#include <string>

/* transform component for scene objects */
struct Transform {
    Vec3 position;
    Vec3 rotation;   /* euler angles in radians */
    Vec3 scale;

    Transform() :
        position(0.0f),
        rotation(0.0f),
        scale(1.0f)
    {}

    /* compute model matrix from transform */
    Mat4 get_matrix() const;
};

/* scene object that can be rendered */
struct SceneObject {
    std::string name;
    Transform transform;
    Mesh* mesh;           /* pointer to mesh data */
    Material material;
    bool visible;
    bool transparent;     /* if true, render with alpha blending */

    SceneObject() :
        name("unnamed"),
        mesh(nullptr),
        visible(true),
        transparent(false)
    {}
};

/* scene containing objects, lights, and camera settings */
class Scene {
    private:
        std::vector<SceneObject> objects;
        std::vector<Light> lights;
        Color ambient_light;

    public:
        Scene();

        /* object management */
        SceneObject& add_object(const std::string& name = "object");
        SceneObject* get_object(const std::string& name);
        std::vector<SceneObject>& get_objects();
        void remove_object(const std::string& name);
        void clear_objects();

        /* light management */
        void add_light(const Light& light);
        std::vector<Light>& get_lights();
        void clear_lights();
        void set_ambient_light(Color color);
        Color get_ambient_light() const;

        /* helpers */
        size_t object_count() const;
        size_t light_count() const;
};
