#include "pipeline/rasterizer.h"
#include <algorithm>
#include <cmath>

Rasterizer::Rasterizer() {
    framebuffer = nullptr;
    fragment_shader = nullptr;
    wireframe_mode = false;
    backface_culling = true;
    blend_mode = BlendMode::NONE;
    depth_write = true;
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
}

void Rasterizer::set_framebuffer(FrameBuffer* fb) {
    framebuffer = fb;
}

void Rasterizer::set_fragment_shader(FragmentShader shader) {
    fragment_shader = shader;
}

void Rasterizer::set_wireframe_mode(bool enabled) {
    wireframe_mode = enabled;
}

void Rasterizer::set_backface_culling(bool enabled) {
    backface_culling = enabled;
}

void Rasterizer::set_blend_mode(BlendMode mode) {
    blend_mode = mode;
}

void Rasterizer::set_depth_write(bool enabled) {
    depth_write = enabled;
}

void Rasterizer::set_num_threads(int threads) {
    if (threads <= 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    } else {
        num_threads = threads;
    }
}

void Rasterizer::write_pixel_safe(int x, int y, float depth, const Color& color) {
    std::lock_guard<std::mutex> lock(framebuffer_mutex);
    float current_depth = framebuffer->get_depth(x, y);
    if (depth < current_depth) {
        if (blend_mode == BlendMode::NONE) {
            framebuffer->set_pixel(x, y, color);
        } else {
            framebuffer->set_pixel_blended(x, y, color, blend_mode);
        }
        if (depth_write) {
            framebuffer->set_depth(x, y, depth);
        }
    }
}

float Rasterizer::edge_function(Vec2 a, Vec2 b, Vec2 c) {
    /* returns (b - a) x (c - a), the 2D cross product */
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

Fragment Rasterizer::interpolate_fragment(Vec3 bary, const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2, Vec3 screen_pos) {
    Fragment frag;

    float w0 = bary.x;
    float w1 = bary.y;
    float w2 = bary.z;

    frag.screen_pos = screen_pos;
    frag.world_pos = v0.world_pos * w0 + v1.world_pos * w1 + v2.world_pos * w2;
    frag.normal = glm::normalize(v0.normal * w0 + v1.normal * w1 + v2.normal * w2);
    frag.tex_coord = v0.tex_coord * w0 + v1.tex_coord * w1 + v2.tex_coord * w2;
    frag.color = v0.color * w0 + v1.color * w1 + v2.color * w2;

    return frag;
}

void Rasterizer::draw_triangle(const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2) {
    if (framebuffer == nullptr) {
        return;
    }

    /* get screen positions */
    Vec2 p0 = Vec2(v0.position.x, v0.position.y);
    Vec2 p1 = Vec2(v1.position.x, v1.position.y);
    Vec2 p2 = Vec2(v2.position.x, v2.position.y);

    /* calculate signed area (2x) using edge function */
    float area = edge_function(p0, p1, p2);

    /* backface culling: if area is negative, triangle faces away */
    if (backface_culling && area < 0) {
        return;
    }

    /* degenerate triangle check */
    if (std::abs(area) < 0.0001f) {
        return;
    }

    /* wireframe mode: just draw edges */
    if (wireframe_mode) {
        draw_line(static_cast<int>(p0.x), static_cast<int>(p0.y),
                  static_cast<int>(p1.x), static_cast<int>(p1.y), v0.color);
        draw_line(static_cast<int>(p1.x), static_cast<int>(p1.y),
                  static_cast<int>(p2.x), static_cast<int>(p2.y), v1.color);
        draw_line(static_cast<int>(p2.x), static_cast<int>(p2.y),
                  static_cast<int>(p0.x), static_cast<int>(p0.y), v2.color);
        return;
    }

    /* compute bounding box of triangle */
    int min_x = static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x})));
    int min_y = static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y})));
    int max_x = static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x})));
    int max_y = static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y})));

    /* clip bounding box to screen */
    min_x = std::max(min_x, 0);
    min_y = std::max(min_y, 0);
    max_x = std::min(max_x, framebuffer->get_width() - 1);
    max_y = std::min(max_y, framebuffer->get_height() - 1);

    /* inverse of area for barycentric normalization */
    float inv_area = 1.0f / area;

    /* rasterize: iterate over all pixels in bounding box */
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            /* sample at pixel center */
            Vec2 p = Vec2(x + 0.5f, y + 0.5f);

            /* compute barycentric coordinates using edge functions */
            float w0 = edge_function(p1, p2, p) * inv_area;
            float w1 = edge_function(p2, p0, p) * inv_area;
            float w2 = edge_function(p0, p1, p) * inv_area;

            /* check if point is inside triangle (all weights >= 0) */
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                /* interpolate depth */
                float depth = w0 * v0.position.z + w1 * v1.position.z + w2 * v2.position.z;

                /* depth test - for blending, still test but may not write */
                float current_depth = framebuffer->get_depth(x, y);
                if (depth < current_depth) {
                    /* create fragment with interpolated attributes */
                    Vec3 bary = Vec3(w0, w1, w2);
                    Vec3 screen_pos = Vec3(x, y, depth);
                    Fragment frag = interpolate_fragment(bary, v0, v1, v2, screen_pos);

                    /* compute final color */
                    Color color;
                    if (fragment_shader) {
                        color = fragment_shader(frag);
                    } else {
                        color = frag.color;
                    }

                    /* write to framebuffer with blending */
                    if (blend_mode == BlendMode::NONE) {
                        framebuffer->set_pixel(x, y, color);
                    } else {
                        framebuffer->set_pixel_blended(x, y, color, blend_mode);
                    }

                    /* update depth buffer if depth writing is enabled */
                    if (depth_write) {
                        framebuffer->set_depth(x, y, depth);
                    }
                }
            }
        }
    }
}

void Rasterizer::draw_line(int x0, int y0, int x1, int y1, Color color) {
    /* midpoint line algorithm - incremental variant */
    int dx = x1 - x0;
    int dy = y1 - y0;

    int step_x = (dx >= 0) ? 1 : -1;
    int step_y = (dy >= 0) ? 1 : -1;

    dx = std::abs(dx);
    dy = std::abs(dy);

    framebuffer->set_pixel(x0, y0, color);

    if (dx >= dy) {
        /* x is the driving axis */
        int d = 2 * dy - dx;
        int incr_e = 2 * dy;
        int incr_ne = 2 * (dy - dx);

        while (x0 != x1) {
            if (d <= 0) {
                d += incr_e;
            } else {
                d += incr_ne;
                y0 += step_y;
            }
            x0 += step_x;
            framebuffer->set_pixel(x0, y0, color);
        }
    } else {
        /* y is the driving axis */
        int d = 2 * dx - dy;
        int incr_e = 2 * dx;
        int incr_ne = 2 * (dx - dy);

        while (y0 != y1) {
            if (d <= 0) {
                d += incr_e;
            } else {
                d += incr_ne;
                x0 += step_x;
            }
            y0 += step_y;
            framebuffer->set_pixel(x0, y0, color);
        }
    }
}

void Rasterizer::draw_triangles_parallel(const std::vector<RasterVertex>& vertices) {
    if (framebuffer == nullptr || vertices.size() < 3) {
        return;
    }

    size_t num_triangles = vertices.size() / 3;
    std::atomic<size_t> triangle_index(0);

    auto worker = [&]() {
        while (true) {
            size_t idx = triangle_index.fetch_add(1);
            if (idx >= num_triangles) break;

            size_t base = idx * 3;
            const RasterVertex& v0 = vertices[base];
            const RasterVertex& v1 = vertices[base + 1];
            const RasterVertex& v2 = vertices[base + 2];

            /* get screen positions */
            Vec2 p0 = Vec2(v0.position.x, v0.position.y);
            Vec2 p1 = Vec2(v1.position.x, v1.position.y);
            Vec2 p2 = Vec2(v2.position.x, v2.position.y);

            /* calculate signed area */
            float area = edge_function(p0, p1, p2);

            /* backface culling */
            if (backface_culling && area < 0) continue;

            /* degenerate triangle check */
            if (std::abs(area) < 0.0001f) continue;

            /* compute bounding box */
            int min_x = static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x})));
            int min_y = static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y})));
            int max_x = static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x})));
            int max_y = static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y})));

            /* clip to screen */
            min_x = std::max(min_x, 0);
            min_y = std::max(min_y, 0);
            max_x = std::min(max_x, framebuffer->get_width() - 1);
            max_y = std::min(max_y, framebuffer->get_height() - 1);

            float inv_area = 1.0f / area;

            /* rasterize */
            for (int y = min_y; y <= max_y; y++) {
                for (int x = min_x; x <= max_x; x++) {
                    Vec2 p = Vec2(x + 0.5f, y + 0.5f);

                    float w0 = edge_function(p1, p2, p) * inv_area;
                    float w1 = edge_function(p2, p0, p) * inv_area;
                    float w2 = edge_function(p0, p1, p) * inv_area;

                    if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                        float depth = w0 * v0.position.z + w1 * v1.position.z + w2 * v2.position.z;

                        Vec3 bary = Vec3(w0, w1, w2);
                        Vec3 screen_pos = Vec3(x, y, depth);
                        Fragment frag = interpolate_fragment(bary, v0, v1, v2, screen_pos);

                        Color color;
                        if (fragment_shader) {
                            color = fragment_shader(frag);
                        } else {
                            color = frag.color;
                        }

                        write_pixel_safe(x, y, depth, color);
                    }
                }
            }
        }
    };

    /* launch worker threads */
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker);
    }

    /* wait for all threads to complete */
    for (auto& t : threads) {
        t.join();
    }
}
