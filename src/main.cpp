#include "framebuffer.h"
#include "camera.h"
#include "output.h"
#include "pipeline/rasterizer.h"
#include "pipeline/vertex_processor.h"
#include "pipeline/clipper.h"
#include <iostream>

/* helper to convert VertexOutput to ClipVertex */
ClipVertex to_clip_vertex(const VertexOutput& v) {
    ClipVertex cv;
    cv.clip_pos = v.clip_pos;
    cv.world_pos = v.world_pos;
    cv.normal = v.normal;
    cv.tex_coord = v.tex_coord;
    cv.color = v.color;
    return cv;
}

/* helper to convert ClipVertex to RasterVertex (with perspective divide) */
RasterVertex to_raster_vertex(const ClipVertex& cv, int viewport_width, int viewport_height) {
    RasterVertex rv;

    /* perspective divide */
    Vec3 ndc;
    if (cv.clip_pos.w != 0) {
        ndc = Vec3(
            cv.clip_pos.x / cv.clip_pos.w,
            cv.clip_pos.y / cv.clip_pos.w,
            cv.clip_pos.z / cv.clip_pos.w
        );
    } else {
        ndc = Vec3(cv.clip_pos.x, cv.clip_pos.y, cv.clip_pos.z);
    }

    /* viewport transform */
    rv.position.x = (ndc.x + 1.0f) * 0.5f * viewport_width;
    rv.position.y = (1.0f - ndc.y) * 0.5f * viewport_height;
    rv.position.z = (ndc.z + 1.0f) * 0.5f;

    rv.world_pos = cv.world_pos;
    rv.normal = cv.normal;
    rv.tex_coord = cv.tex_coord;
    rv.color = cv.color;

    return rv;
}

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    /* create framebuffer */
    FrameBuffer framebuffer(WIDTH, HEIGHT);

    /* create camera */
    Camera camera;
    camera.set_position(Vec3(0.0f, 0.0f, 3.0f));
    camera.set_target(Vec3(0.0f, 0.0f, 0.0f));
    camera.set_perspective(glm::radians(45.0f), static_cast<float>(WIDTH) / HEIGHT, 0.1f, 100.0f);

    /* create pipeline components */
    VertexProcessor vertex_processor;
    vertex_processor.set_viewport(WIDTH, HEIGHT);
    vertex_processor.set_camera(camera);
    vertex_processor.set_model_matrix(Mat4(1.0f));

    Clipper clipper;

    Rasterizer rasterizer;
    rasterizer.set_framebuffer(&framebuffer);
    rasterizer.set_backface_culling(false);  /* disable for testing */

    /* clear framebuffer */
    framebuffer.clear(Color(0.1f, 0.1f, 0.2f, 1.0f));
    framebuffer.clear_depth(1.0f);

    /* define a simple triangle */
    VertexInput v0, v1, v2;

    v0.position = Vec3(0.0f, 0.5f, 0.0f);
    v0.normal = Vec3(0.0f, 0.0f, 1.0f);
    v0.tex_coord = Vec2(0.5f, 1.0f);
    v0.color = Colors::red();

    v1.position = Vec3(-0.5f, -0.5f, 0.0f);
    v1.normal = Vec3(0.0f, 0.0f, 1.0f);
    v1.tex_coord = Vec2(0.0f, 0.0f);
    v1.color = Colors::green();

    v2.position = Vec3(0.5f, -0.5f, 0.0f);
    v2.normal = Vec3(0.0f, 0.0f, 1.0f);
    v2.tex_coord = Vec2(1.0f, 0.0f);
    v2.color = Colors::blue();

    /* vertex processing stage */
    VertexOutput out0 = vertex_processor.process_vertex(v0);
    VertexOutput out1 = vertex_processor.process_vertex(v1);
    VertexOutput out2 = vertex_processor.process_vertex(v2);

    /* clipping stage */
    ClipVertex cv0 = to_clip_vertex(out0);
    ClipVertex cv1 = to_clip_vertex(out1);
    ClipVertex cv2 = to_clip_vertex(out2);

    std::vector<ClipVertex> clipped = clipper.clip_triangle(cv0, cv1, cv2);

    /* rasterize resulting triangles */
    for (size_t i = 0; i + 2 < clipped.size(); i += 3) {
        RasterVertex rv0 = to_raster_vertex(clipped[i], WIDTH, HEIGHT);
        RasterVertex rv1 = to_raster_vertex(clipped[i + 1], WIDTH, HEIGHT);
        RasterVertex rv2 = to_raster_vertex(clipped[i + 2], WIDTH, HEIGHT);

        rasterizer.draw_triangle(rv0, rv1, rv2);
    }

    /* save output */
    if (Output::save(framebuffer, "output/render.ppm")) {
        std::cout << "Render saved to output/render.ppm" << std::endl;
    } else {
        std::cout << "Failed to save render" << std::endl;
    }

    return 0;
}
