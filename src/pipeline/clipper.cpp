#include "pipeline/clipper.h"

Clipper::Clipper() {
}

bool Clipper::is_inside_plane(const Vec4& clip_pos, ClipPlane plane) {
    float x = clip_pos.x;
    float y = clip_pos.y;
    float z = clip_pos.z;
    float w = clip_pos.w;

    switch (plane) {
        case ClipPlane::LEFT:   return x >= -w;
        case ClipPlane::RIGHT:  return x <= w;
        case ClipPlane::BOTTOM: return y >= -w;
        case ClipPlane::TOP:    return y <= w;
        case ClipPlane::NEAR:   return z >= -w;
        case ClipPlane::FAR:    return z <= w;
    }
    return true;
}

float Clipper::intersect_plane(const ClipVertex& v0, const ClipVertex& v1, ClipPlane plane) {
    /* find t where the edge v0->v1 crosses the clip plane */
    Vec4 p0 = v0.clip_pos;
    Vec4 p1 = v1.clip_pos;

    float d0, d1;

    switch (plane) {
        case ClipPlane::LEFT:
            d0 = p0.x + p0.w;
            d1 = p1.x + p1.w;
            break;
        case ClipPlane::RIGHT:
            d0 = p0.w - p0.x;
            d1 = p1.w - p1.x;
            break;
        case ClipPlane::BOTTOM:
            d0 = p0.y + p0.w;
            d1 = p1.y + p1.w;
            break;
        case ClipPlane::TOP:
            d0 = p0.w - p0.y;
            d1 = p1.w - p1.y;
            break;
        case ClipPlane::NEAR:
            d0 = p0.z + p0.w;
            d1 = p1.z + p1.w;
            break;
        case ClipPlane::FAR:
            d0 = p0.w - p0.z;
            d1 = p1.w - p1.z;
            break;
    }

    /* t = d0 / (d0 - d1) */
    return d0 / (d0 - d1);
}

ClipVertex Clipper::interpolate_vertex(const ClipVertex& v0, const ClipVertex& v1, float t) {
    ClipVertex result;

    /* linear interpolation of all attributes */
    result.clip_pos = v0.clip_pos + (v1.clip_pos - v0.clip_pos) * t;
    result.world_pos = v0.world_pos + (v1.world_pos - v0.world_pos) * t;
    result.normal = glm::normalize(v0.normal + (v1.normal - v0.normal) * t);
    result.tex_coord = v0.tex_coord + (v1.tex_coord - v0.tex_coord) * t;
    result.color = v0.color + (v1.color - v0.color) * t;

    return result;
}

std::vector<ClipVertex> Clipper::clip_polygon_against_plane(const std::vector<ClipVertex>& vertices, ClipPlane plane) {
    /* Sutherland-Hodgman polygon clipping */
    if (vertices.empty()) {
        return {};
    }

    std::vector<ClipVertex> result;

    for (size_t i = 0; i < vertices.size(); i++) {
        const ClipVertex& current = vertices[i];
        const ClipVertex& next = vertices[(i + 1) % vertices.size()];

        bool current_inside = is_inside_plane(current.clip_pos, plane);
        bool next_inside = is_inside_plane(next.clip_pos, plane);

        if (current_inside) {
            if (next_inside) {
                /* both inside: keep next vertex */
                result.push_back(next);
            } else {
                /* current inside, next outside: add intersection */
                float t = intersect_plane(current, next, plane);
                result.push_back(interpolate_vertex(current, next, t));
            }
        } else {
            if (next_inside) {
                /* current outside, next inside: add intersection and next */
                float t = intersect_plane(current, next, plane);
                result.push_back(interpolate_vertex(current, next, t));
                result.push_back(next);
            }
            /* both outside: add nothing */
        }
    }

    return result;
}

std::vector<ClipVertex> Clipper::clip_triangle(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2) {
    /* start with the input triangle as a polygon */
    std::vector<ClipVertex> polygon = {v0, v1, v2};

    /* clip against each frustum plane (Sutherland-Hodgman) */
    static const ClipPlane planes[] = {
        ClipPlane::NEAR,    /* clip near first to avoid w <= 0 issues */
        ClipPlane::FAR,
        ClipPlane::LEFT,
        ClipPlane::RIGHT,
        ClipPlane::BOTTOM,
        ClipPlane::TOP
    };

    for (ClipPlane plane : planes) {
        polygon = clip_polygon_against_plane(polygon, plane);

        /* if polygon is completely clipped away, return empty */
        if (polygon.size() < 3) {
            return {};
        }
    }

    /* triangulate the resulting polygon (fan triangulation) */
    std::vector<ClipVertex> result;

    for (size_t i = 1; i < polygon.size() - 1; i++) {
        result.push_back(polygon[0]);
        result.push_back(polygon[i]);
        result.push_back(polygon[i + 1]);
    }

    return result;
}

bool Clipper::is_inside_frustum(const Vec4& clip_pos) {
    float x = clip_pos.x;
    float y = clip_pos.y;
    float z = clip_pos.z;
    float w = clip_pos.w;

    return x >= -w && x <= w &&
           y >= -w && y <= w &&
           z >= -w && z <= w;
}

bool Clipper::is_triangle_outside(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2) {
    /* trivial rejection: if all vertices are outside the same plane */
    static const ClipPlane planes[] = {
        ClipPlane::LEFT, ClipPlane::RIGHT,
        ClipPlane::BOTTOM, ClipPlane::TOP,
        ClipPlane::NEAR, ClipPlane::FAR
    };

    for (ClipPlane plane : planes) {
        if (!is_inside_plane(v0.clip_pos, plane) &&
            !is_inside_plane(v1.clip_pos, plane) &&
            !is_inside_plane(v2.clip_pos, plane)) {
            return true;
        }
    }

    return false;
}
