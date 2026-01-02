#pragma once

#include "math/vector.h"
#include <vector>

/* clip planes in clip space */
enum class ClipPlane {
    LEFT,       /* x >= -w */
    RIGHT,      /* x <= w */
    BOTTOM,     /* y >= -w */
    TOP,        /* y <= w */
    NEAR,       /* z >= -w */
    FAR         /* z <= w */
};

/* vertex with clip space position and attributes for interpolation */
struct ClipVertex {
    Vec4 clip_pos;      /* position in clip space */
    Vec3 world_pos;     /* world position for lighting */
    Vec3 normal;        /* normal vector */
    Vec2 tex_coord;     /* texture coordinates */
    Color color;        /* vertex color */
};

class Clipper {
    private:
        /* clip a polygon against a single plane */
        std::vector<ClipVertex> clip_polygon_against_plane(const std::vector<ClipVertex>& vertices, ClipPlane plane);

        /* check if a point is inside a specific clip plane */
        bool is_inside_plane(const Vec4& clip_pos, ClipPlane plane);

        /* compute intersection parameter t where edge crosses clip plane */
        float intersect_plane(const ClipVertex& v0, const ClipVertex& v1, ClipPlane plane);

        /* interpolate vertex attributes at parameter t */
        ClipVertex interpolate_vertex(const ClipVertex& v0, const ClipVertex& v1, float t);

    public:
        /* constructor */
        Clipper();

        /* clip a triangle against all frustum planes */
        /* returns clipped vertices (0, 3, 6, ... for 0, 1, 2, ... triangles) */
        std::vector<ClipVertex> clip_triangle(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2);

        /* check if a point is inside the view frustum */
        bool is_inside_frustum(const Vec4& clip_pos);

        /* check if a triangle is completely outside (trivial reject) */
        bool is_triangle_outside(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2);
};
