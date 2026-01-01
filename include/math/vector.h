#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/* type aliases for convenience */
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Color = glm::vec4;

using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;

/* color helpers */
namespace Colors {
    inline Color black()   { return Color(0, 0, 0, 1); }
    inline Color white()   { return Color(1, 1, 1, 1); }
    inline Color red()     { return Color(1, 0, 0, 1); }
    inline Color green()   { return Color(0, 1, 0, 1); }
    inline Color blue()    { return Color(0, 0, 1, 1); }
    inline Color yellow()  { return Color(1, 1, 0, 1); }
    inline Color cyan()    { return Color(0, 1, 1, 1); }
    inline Color magenta() { return Color(1, 0, 1, 1); }
}
