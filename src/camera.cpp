#include "camera.h"

Camera::Camera() {
    position = Vec3(0.0f, 0.0f, 5.0f);
    target = Vec3(0.0f, 0.0f, 0.0f);
    up = Vec3(0.0f, 1.0f, 0.0f);

    fov = glm::radians(45.0f);
    aspect_ratio = 16.0f / 9.0f;
    near_plane = 0.1f;
    far_plane = 100.0f;

    dirty = true;
}

void Camera::set_position(Vec3 i_position) {
    position = i_position;
    dirty = true;
}

void Camera::set_target(Vec3 i_target) {
    target = i_target;
    dirty = true;
}

void Camera::set_up(Vec3 i_up) {
    up = i_up;
    dirty = true;
}

void Camera::set_fov(float i_fov) {
    fov = i_fov;
    dirty = true;
}

void Camera::set_aspect_ratio(float i_aspect_ratio) {
    aspect_ratio = i_aspect_ratio;
    dirty = true;
}

void Camera::set_near_plane(float i_near_plane) {
    near_plane = i_near_plane;
    dirty = true;
}

void Camera::set_far_plane(float i_far_plane) {
    far_plane = i_far_plane;
    dirty = true;
}

void Camera::set_perspective(float i_fov, float i_aspect_ratio, float i_near_plane, float i_far_plane) {
    fov = i_fov;
    aspect_ratio = i_aspect_ratio;
    near_plane = i_near_plane;
    far_plane = i_far_plane;
    dirty = true;
}

void Camera::look_at(Vec3 eye, Vec3 center, Vec3 i_up) {
    position = eye;
    target = center;
    up = i_up;
    dirty = true;
}

Vec3 Camera::get_position() {
    return position;
}

Vec3 Camera::get_target() {
    return target;
}

Vec3 Camera::get_forward() {
    return glm::normalize(target - position);
}

Vec3 Camera::get_right() {
    return glm::normalize(glm::cross(get_forward(), up));
}

Vec3 Camera::get_up() {
    return glm::normalize(up);
}

Mat4 Camera::get_view_matrix() {
    if (dirty) {
        update_matrices();
    }
    return view_matrix;
}

Mat4 Camera::get_projection_matrix() {
    if (dirty) {
        update_matrices();
    }
    return proj_matrix;
}

Mat4 Camera::get_view_projection_matrix() {
    if (dirty) {
        update_matrices();
    }
    return view_proj_matrix;
}

void Camera::update_matrices() {
    view_matrix = glm::lookAt(position, target, up);
    proj_matrix = glm::perspective(fov, aspect_ratio, near_plane, far_plane);
    view_proj_matrix = proj_matrix * view_matrix;
    dirty = false;
}
