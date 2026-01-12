#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>

Mat4 Transform::get_matrix() const {
    Mat4 mat(1.0f);

    /* apply transformations: scale -> rotate -> translate */
    mat = glm::translate(mat, position);
    mat = glm::rotate(mat, rotation.x, Vec3(1.0f, 0.0f, 0.0f));
    mat = glm::rotate(mat, rotation.y, Vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, rotation.z, Vec3(0.0f, 0.0f, 1.0f));
    mat = glm::scale(mat, scale);

    return mat;
}

Scene::Scene() :
    ambient_light(0.1f, 0.1f, 0.1f, 1.0f)
{}

SceneObject& Scene::add_object(const std::string& name) {
    SceneObject obj;
    obj.name = name;
    objects.push_back(obj);
    return objects.back();
}

SceneObject* Scene::get_object(const std::string& name) {
    for (auto& obj : objects) {
        if (obj.name == name) {
            return &obj;
        }
    }
    return nullptr;
}

std::vector<SceneObject>& Scene::get_objects() {
    return objects;
}

void Scene::remove_object(const std::string& name) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->name == name) {
            objects.erase(it);
            return;
        }
    }
}

void Scene::clear_objects() {
    objects.clear();
}

void Scene::add_light(const Light& light) {
    lights.push_back(light);
}

std::vector<Light>& Scene::get_lights() {
    return lights;
}

void Scene::clear_lights() {
    lights.clear();
}

void Scene::set_ambient_light(Color color) {
    ambient_light = color;
}

Color Scene::get_ambient_light() const {
    return ambient_light;
}

size_t Scene::object_count() const {
    return objects.size();
}

size_t Scene::light_count() const {
    return lights.size();
}
