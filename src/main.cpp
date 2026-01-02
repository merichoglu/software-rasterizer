#include "framebuffer.h"
#include "camera.h"
#include "output.h"
#include "pipeline/rasterizer.h"
#include "pipeline/vertex_processor.h"
#include <iostream>

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

    /* process vertices */
    VertexOutput out0 = vertex_processor.process_vertex(v0);
    VertexOutput out1 = vertex_processor.process_vertex(v1);
    VertexOutput out2 = vertex_processor.process_vertex(v2);

    /* create raster vertices */
    RasterVertex rv0, rv1, rv2;

    rv0.position = out0.screen_pos;
    rv0.world_pos = out0.world_pos;
    rv0.normal = out0.normal;
    rv0.tex_coord = out0.tex_coord;
    rv0.color = out0.color;

    rv1.position = out1.screen_pos;
    rv1.world_pos = out1.world_pos;
    rv1.normal = out1.normal;
    rv1.tex_coord = out1.tex_coord;
    rv1.color = out1.color;

    rv2.position = out2.screen_pos;
    rv2.world_pos = out2.world_pos;
    rv2.normal = out2.normal;
    rv2.tex_coord = out2.tex_coord;
    rv2.color = out2.color;

    /* rasterize triangle */
    rasterizer.draw_triangle(rv0, rv1, rv2);

    /* save output */
    if (Output::save(framebuffer, "output/render.ppm")) {
        std::cout << "Render saved to output/render.ppm" << std::endl;
    } else {
        std::cout << "Failed to save render" << std::endl;
    }

    return 0;
}
