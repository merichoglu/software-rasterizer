#pragma once

#include "vector.h"
#include <glm/gtc/matrix_transform.hpp>

/* type aliases for convenience */
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

/* matrix construction helpers */
namespace MatrixUtils {
    inline Mat4 identity() {
        return Mat4(1.0f);
    }

    inline Mat4 translate(const Vec3& v) {
        return glm::translate(Mat4(1.0f), v);
    }

    inline Mat4 scale(const Vec3& v) {
        return glm::scale(Mat4(1.0f), v);
    }

    inline Mat4 scale(float s) {
        return glm::scale(Mat4(1.0f), Vec3(s));
    }

    inline Mat4 rotateX(float radians) {
        return glm::rotate(Mat4(1.0f), radians, Vec3(1, 0, 0));
    }

    inline Mat4 rotateY(float radians) {
        return glm::rotate(Mat4(1.0f), radians, Vec3(0, 1, 0));
    }

    inline Mat4 rotateZ(float radians) {
        return glm::rotate(Mat4(1.0f), radians, Vec3(0, 0, 1));
    }

    inline Mat4 rotate(float radians, const Vec3& axis) {
        return glm::rotate(Mat4(1.0f), radians, axis);
    }

    inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        return glm::lookAt(eye, center, up);
    }

    inline Mat4 perspective(float fovY, float aspect, float near, float far) {
        return glm::perspective(fovY, aspect, near, far);
    }

    inline Mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        return glm::ortho(left, right, bottom, top, near, far);
    }
}
