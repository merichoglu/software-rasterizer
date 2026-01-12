#include "framebuffer.h"
#include "camera.h"
#include "output.h"
#include "model_loader.h"
#include "texture.h"
#include "scene.h"
#include "pipeline/rasterizer.h"
#include "pipeline/vertex_processor.h"
#include "pipeline/clipper.h"
#include "pipeline/fragment_processor.h"
#include "pipeline/shadow_map.h"
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

/* render a mesh through the pipeline */
void render_mesh(const Mesh& mesh, VertexProcessor& vertex_processor, Clipper& clipper,
                 Rasterizer& rasterizer, int width, int height) {
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const VertexInput& v0 = mesh.vertices[mesh.indices[i]];
        const VertexInput& v1 = mesh.vertices[mesh.indices[i + 1]];
        const VertexInput& v2 = mesh.vertices[mesh.indices[i + 2]];

        VertexOutput out0 = vertex_processor.process_vertex(v0);
        VertexOutput out1 = vertex_processor.process_vertex(v1);
        VertexOutput out2 = vertex_processor.process_vertex(v2);

        ClipVertex cv0 = to_clip_vertex(out0);
        ClipVertex cv1 = to_clip_vertex(out1);
        ClipVertex cv2 = to_clip_vertex(out2);

        std::vector<ClipVertex> clipped = clipper.clip_triangle(cv0, cv1, cv2);

        for (size_t j = 0; j + 2 < clipped.size(); j += 3) {
            RasterVertex rv0 = to_raster_vertex(clipped[j], width, height);
            RasterVertex rv1 = to_raster_vertex(clipped[j + 1], width, height);
            RasterVertex rv2 = to_raster_vertex(clipped[j + 2], width, height);

            rasterizer.draw_triangle(rv0, rv1, rv2);
        }
    }
}

/* render shadow pass - depth only from light's perspective */
void render_shadow_pass(Scene& scene, ShadowMap& shadow_map, Clipper& clipper) {
    int width = shadow_map.get_width();
    int height = shadow_map.get_height();
    Mat4 light_space = shadow_map.get_light_space_matrix();

    for (SceneObject& obj : scene.get_objects()) {
        if (!obj.visible || !obj.mesh) {
            continue;
        }

        Mat4 model = obj.transform.get_matrix();
        Mat4 mvp = light_space * model;

        for (size_t i = 0; i < obj.mesh->indices.size(); i += 3) {
            const VertexInput& v0 = obj.mesh->vertices[obj.mesh->indices[i]];
            const VertexInput& v1 = obj.mesh->vertices[obj.mesh->indices[i + 1]];
            const VertexInput& v2 = obj.mesh->vertices[obj.mesh->indices[i + 2]];

            /* transform to light clip space */
            Vec4 clip0 = mvp * Vec4(v0.position, 1.0f);
            Vec4 clip1 = mvp * Vec4(v1.position, 1.0f);
            Vec4 clip2 = mvp * Vec4(v2.position, 1.0f);

            /* create clip vertices for clipping */
            ClipVertex cv0, cv1, cv2;
            cv0.clip_pos = clip0;
            cv1.clip_pos = clip1;
            cv2.clip_pos = clip2;

            std::vector<ClipVertex> clipped = clipper.clip_triangle(cv0, cv1, cv2);

            /* rasterize depth only */
            for (size_t j = 0; j + 2 < clipped.size(); j += 3) {
                /* perspective divide and viewport transform */
                auto to_screen = [&](const ClipVertex& cv) -> Vec3 {
                    Vec3 ndc;
                    if (cv.clip_pos.w != 0) {
                        ndc = Vec3(cv.clip_pos) / cv.clip_pos.w;
                    } else {
                        ndc = Vec3(cv.clip_pos);
                    }
                    return Vec3(
                        (ndc.x + 1.0f) * 0.5f * width,
                        (1.0f - ndc.y) * 0.5f * height,
                        (ndc.z + 1.0f) * 0.5f
                    );
                };

                Vec3 p0 = to_screen(clipped[j]);
                Vec3 p1 = to_screen(clipped[j + 1]);
                Vec3 p2 = to_screen(clipped[j + 2]);

                /* simple triangle rasterization for depth */
                int min_x = std::max(0, static_cast<int>(std::min({p0.x, p1.x, p2.x})));
                int max_x = std::min(width - 1, static_cast<int>(std::max({p0.x, p1.x, p2.x})));
                int min_y = std::max(0, static_cast<int>(std::min({p0.y, p1.y, p2.y})));
                int max_y = std::min(height - 1, static_cast<int>(std::max({p0.y, p1.y, p2.y})));

                /* edge function for barycentric coordinates */
                auto edge = [](Vec3 a, Vec3 b, Vec3 c) -> float {
                    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
                };

                float area = edge(p0, p1, p2);
                if (std::abs(area) < 0.001f) continue;

                for (int y = min_y; y <= max_y; ++y) {
                    for (int x = min_x; x <= max_x; ++x) {
                        Vec3 p(x + 0.5f, y + 0.5f, 0.0f);
                        float w0 = edge(p1, p2, p) / area;
                        float w1 = edge(p2, p0, p) / area;
                        float w2 = edge(p0, p1, p) / area;

                        if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                            float depth = w0 * p0.z + w1 * p1.z + w2 * p2.z;
                            shadow_map.depth_test(x, y, depth);
                        }
                    }
                }
            }
        }
    }
}

/* render entire scene */
void render_scene(Scene& scene, FrameBuffer& framebuffer,
                  VertexProcessor& vertex_processor, Clipper& clipper,
                  Rasterizer& rasterizer, FragmentProcessor& fragment_processor,
                  bool transparent_pass) {
    int width = framebuffer.get_width();
    int height = framebuffer.get_height();

    /* setup lights from scene */
    fragment_processor.clear_lights();
    fragment_processor.set_ambient_light(scene.get_ambient_light());
    for (const Light& light : scene.get_lights()) {
        fragment_processor.add_light(light);
    }

    /* configure rasterizer for this pass */
    if (transparent_pass) {
        rasterizer.set_blend_mode(BlendMode::ALPHA);
        rasterizer.set_depth_write(false);
    } else {
        rasterizer.set_blend_mode(BlendMode::NONE);
        rasterizer.set_depth_write(true);
    }

    /* render each visible object */
    for (SceneObject& obj : scene.get_objects()) {
        if (!obj.visible || !obj.mesh) {
            continue;
        }

        /* skip objects based on pass type */
        if (transparent_pass != obj.transparent) {
            continue;
        }

        /* set object's model matrix */
        vertex_processor.set_model_matrix(obj.transform.get_matrix());

        /* set object's material */
        fragment_processor.set_material(obj.material);

        /* render the mesh */
        render_mesh(*obj.mesh, vertex_processor, clipper, rasterizer, width, height);
    }
}

/* create a quad mesh */
Mesh create_quad_mesh(float size, Vec3 normal) {
    Mesh mesh;
    mesh.name = "quad";

    /* determine tangent vectors based on normal */
    Vec3 tangent, bitangent;
    if (std::abs(normal.y) > 0.9f) {
        tangent = Vec3(1.0f, 0.0f, 0.0f);
        bitangent = Vec3(0.0f, 0.0f, normal.y > 0 ? 1.0f : -1.0f);
    } else {
        tangent = glm::normalize(glm::cross(Vec3(0.0f, 1.0f, 0.0f), normal));
        bitangent = glm::cross(normal, tangent);
    }

    /* create 4 vertices */
    VertexInput v0, v1, v2, v3;

    v0.position = (-tangent - bitangent) * size;
    v0.normal = normal;
    v0.tex_coord = Vec2(0.0f, 0.0f);
    v0.color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    v1.position = (tangent - bitangent) * size;
    v1.normal = normal;
    v1.tex_coord = Vec2(1.0f, 0.0f);
    v1.color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    v2.position = (tangent + bitangent) * size;
    v2.normal = normal;
    v2.tex_coord = Vec2(1.0f, 1.0f);
    v2.color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    v3.position = (-tangent + bitangent) * size;
    v3.normal = normal;
    v3.tex_coord = Vec2(0.0f, 1.0f);
    v3.color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    mesh.vertices = {v0, v1, v2, v3};
    mesh.indices = {0, 2, 1, 0, 3, 2};  /* two triangles */

    return mesh;
}

/* draw sky gradient directly to framebuffer */
void draw_sky(FrameBuffer& framebuffer, Texture& sky_texture) {
    int width = framebuffer.get_width();
    int height = framebuffer.get_height();

    for (int y = 0; y < height; ++y) {
        /* sample sky texture based on vertical position */
        float v = static_cast<float>(y) / height;
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / width;
            Color sky_color = sky_texture.sample(u, v);
            framebuffer.set_pixel(x, y, sky_color);
        }
    }
}

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    /* load teapot model */
    Model teapot_model;
    if (!ModelLoader::load_obj("assets/models/teapot.obj", teapot_model)) {
        std::cerr << "Failed to load teapot model" << std::endl;
        return 1;
    }

    for (auto& mesh : teapot_model.meshes) {
        ModelLoader::compute_smooth_normals(mesh);
    }

    /* create floor mesh - larger for better ground coverage */
    Mesh floor_mesh = create_quad_mesh(25.0f, Vec3(0.0f, 1.0f, 0.0f));

    /* scale floor UVs for tiling */
    for (auto& v : floor_mesh.vertices) {
        v.tex_coord *= 4.0f;
    }

    /* load textures */
    Texture sky_texture;
    if (!sky_texture.load("assets/textures/scattered-clouds-blue-sky.jpg")) {
        std::cerr << "Failed to load sky texture, using fallback" << std::endl;
        sky_texture.create_solid(1, 1, Color(0.4f, 0.6f, 0.9f, 1.0f));
    }
    sky_texture.set_wrap_mode(WrapMode::CLAMP_TO_EDGE);

    /* use a nice checkerboard for the ground - more visually appealing */
    Texture ground_texture;
    if (!ground_texture.load("assets/textures/green-grass-background.jpg")) {
        std::cerr << "Failed to load ground texture, using fallback" << std::endl;
        ground_texture.create_solid(1, 1, Color(0.4f, 0.6f, 0.9f, 1.0f));
    }
    ground_texture.set_wrap_mode(WrapMode::REPEAT);

    /* create scene */
    Scene scene;
    scene.set_ambient_light(Color(0.15f, 0.15f, 0.2f, 1.0f));

    /* add directional light (sun) - warm light from upper right */
    Light sun;
    sun.type = LightType::DIRECTIONAL;
    sun.direction = Vec3(-0.5f, -1.0f, -0.3f);
    sun.color = Color(1.0f, 0.95f, 0.85f, 1.0f);  /* warm sunlight */
    sun.intensity = 1.2f;
    scene.add_light(sun);

    /* add fill light from opposite side */
    Light fill;
    fill.type = LightType::DIRECTIONAL;
    fill.direction = Vec3(0.5f, -0.3f, 0.5f);
    fill.color = Color(0.6f, 0.7f, 0.9f, 1.0f);  /* cool sky bounce */
    fill.intensity = 0.3f;
    scene.add_light(fill);

    /* add ground */
    SceneObject& floor_obj = scene.add_object("ground");
    floor_obj.mesh = &floor_mesh;
    floor_obj.material.ambient = Color(0.15f, 0.12f, 0.1f, 1.0f);
    floor_obj.material.diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    floor_obj.material.specular = Color(0.1f, 0.1f, 0.1f, 1.0f);
    floor_obj.material.shininess = 8.0f;
    floor_obj.material.diffuse_map = &ground_texture;

    /* add main teapot (center) - polished copper */
    SceneObject& teapot1 = scene.add_object("teapot_center");
    teapot1.mesh = &teapot_model.meshes[0];
    teapot1.material.ambient = Color(0.19f, 0.07f, 0.02f, 1.0f);
    teapot1.material.diffuse = Color(0.7f, 0.27f, 0.08f, 1.0f);
    teapot1.material.specular = Color(0.95f, 0.64f, 0.54f, 1.0f);
    teapot1.material.shininess = 51.2f;

    /* add second teapot (left) - polished silver */
    SceneObject& teapot2 = scene.add_object("teapot_left");
    teapot2.mesh = &teapot_model.meshes[0];
    teapot2.transform.position = Vec3(-6.0f, 0.0f, 2.0f);
    teapot2.transform.scale = Vec3(0.7f);
    teapot2.transform.rotation = Vec3(0.0f, glm::radians(-30.0f), 0.0f);
    teapot2.material.ambient = Color(0.19f, 0.19f, 0.19f, 1.0f);
    teapot2.material.diffuse = Color(0.51f, 0.51f, 0.51f, 1.0f);
    teapot2.material.specular = Color(0.77f, 0.77f, 0.77f, 1.0f);
    teapot2.material.shininess = 89.6f;

    /* add third teapot (right) - transparent green glass */
    SceneObject& teapot3 = scene.add_object("teapot_right");
    teapot3.mesh = &teapot_model.meshes[0];
    teapot3.transform.position = Vec3(6.0f, 0.0f, 2.0f);
    teapot3.transform.scale = Vec3(0.7f);
    teapot3.transform.rotation = Vec3(0.0f, glm::radians(30.0f), 0.0f);
    teapot3.material.ambient = Color(0.1f, 0.15f, 0.1f, 0.5f);
    teapot3.material.diffuse = Color(0.2f, 0.5f, 0.25f, 0.5f);
    teapot3.material.specular = Color(0.9f, 0.95f, 0.9f, 1.0f);
    teapot3.material.shininess = 96.0f;
    teapot3.transparent = true;

    std::cout << "Scene: " << scene.object_count() << " objects, " << scene.light_count() << " lights" << std::endl;

    /* create framebuffer */
    FrameBuffer framebuffer(WIDTH, HEIGHT);

    /* create camera - lower angle for more dramatic view */
    Camera camera;
    camera.set_position(Vec3(0.0f, 5.0f, 16.0f));
    camera.set_target(Vec3(0.0f, 2.5f, 0.0f));
    camera.set_perspective(glm::radians(50.0f), static_cast<float>(WIDTH) / HEIGHT, 0.1f, 100.0f);

    /* create shadow map */
    const int SHADOW_MAP_SIZE = 1024;
    ShadowMap shadow_map(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    shadow_map.setup_directional_light(sun.direction, Vec3(0.0f, 2.0f, 0.0f), 20.0f);
    shadow_map.set_bias(0.005f);

    /* create pipeline components */
    VertexProcessor vertex_processor;
    vertex_processor.set_viewport(WIDTH, HEIGHT);
    vertex_processor.set_camera(camera);

    Clipper clipper;

    FragmentProcessor fragment_processor;
    fragment_processor.set_camera_position(camera.get_position());
    fragment_processor.set_shadow_map(&shadow_map);
    fragment_processor.enable_shadows(true);

    Rasterizer rasterizer;
    rasterizer.set_framebuffer(&framebuffer);
    rasterizer.set_backface_culling(true);
    rasterizer.set_fragment_shader([&](const Fragment& frag) {
        return fragment_processor.process_fragment(frag);
    });

    /* render shadow pass first */
    std::cout << "Rendering shadow map..." << std::endl;
    render_shadow_pass(scene, shadow_map, clipper);

    /* draw sky background */
    std::cout << "Drawing sky..." << std::endl;
    draw_sky(framebuffer, sky_texture);

    /* clear only depth buffer (keep sky) */
    framebuffer.clear_depth(1.0f);

    /* render opaque objects first */
    std::cout << "Rendering opaque objects..." << std::endl;
    render_scene(scene, framebuffer, vertex_processor, clipper, rasterizer, fragment_processor, false);

    /* render transparent objects with alpha blending */
    std::cout << "Rendering transparent objects..." << std::endl;
    render_scene(scene, framebuffer, vertex_processor, clipper, rasterizer, fragment_processor, true);

    /* save output */
    if (Output::save(framebuffer, "output/render.ppm")) {
        std::cout << "Render saved to output/render.ppm" << std::endl;
    } else {
        std::cout << "Failed to save render" << std::endl;
    }

    return 0;
}
