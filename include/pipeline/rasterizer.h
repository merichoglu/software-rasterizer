#pragma once

#include "math/vector.h"
#include "framebuffer.h"
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/* vertex data for rasterization (screen space) */
struct RasterVertex {
    Vec3 position;      /* screen x, y and depth z */
    Vec3 world_pos;     /* world position for lighting */
    Vec3 normal;        /* normal vector */
    Vec2 tex_coord;     /* texture coordinates */
    Color color;        /* vertex color */
};

/* interpolated fragment data */
struct Fragment {
    Vec3 screen_pos;    /* screen x, y and depth z */
    Vec3 world_pos;     /* interpolated world position */
    Vec3 normal;        /* interpolated normal */
    Vec2 tex_coord;     /* interpolated texture coordinates */
    Color color;        /* interpolated vertex color */
};

/* fragment shader callback type */
using FragmentShader = std::function<Color(const Fragment&)>;

class Rasterizer {
    private:
        FrameBuffer* framebuffer;
        FragmentShader fragment_shader;
        bool wireframe_mode;
        bool backface_culling;
        BlendMode blend_mode;
        bool depth_write;
        int num_threads;
        std::mutex framebuffer_mutex;

        /* calculate edge function for point against edge */
        float edge_function(Vec2 a, Vec2 b, Vec2 c);

        /* interpolate fragment attributes using barycentric coordinates */
        Fragment interpolate_fragment(Vec3 bary, const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2, Vec3 screen_pos);

        /* thread-safe pixel write */
        void write_pixel_safe(int x, int y, float depth, const Color& color);

    public:
        /* constructor */
        Rasterizer();

        /* set target framebuffer */
        void set_framebuffer(FrameBuffer* fb);

        /* set fragment shader callback */
        void set_fragment_shader(FragmentShader shader);

        /* enable/disable wireframe mode */
        void set_wireframe_mode(bool enabled);

        /* enable/disable backface culling */
        void set_backface_culling(bool enabled);

        /* set blend mode for alpha blending */
        void set_blend_mode(BlendMode mode);

        /* enable/disable depth writing (disable for transparent objects) */
        void set_depth_write(bool enabled);

        /* set number of threads for parallel rendering (0 = auto-detect) */
        void set_num_threads(int threads);

        /* rasterize a single triangle */
        void draw_triangle(const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2);

        /* rasterize multiple triangles in parallel */
        void draw_triangles_parallel(const std::vector<RasterVertex>& vertices);

        /* draw a line between two points (for wireframe) */
        void draw_line(int x0, int y0, int x1, int y1, Color color);
};
