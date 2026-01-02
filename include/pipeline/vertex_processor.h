#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "camera.h"

/* input vertex from mesh */
struct VertexInput {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coord;
    Color color;
};

/* output vertex after vertex processing */
struct VertexOutput {
    Vec4 clip_pos;      /* position in clip space (before perspective divide) */
    Vec3 ndc_pos;       /* position in NDC (after perspective divide) */
    Vec3 screen_pos;    /* position in screen space */
    Vec3 world_pos;     /* position in world space (for lighting) */
    Vec3 normal;        /* normal in world space */
    Vec2 tex_coord;     /* passed through texture coordinates */
    Color color;        /* passed through or computed color */
};

/* uniform data for vertex processing */
struct Uniforms {
    Mat4 model_matrix;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 mvp_matrix;
    Mat3 normal_matrix;
};

class VertexProcessor {
    private:
        Uniforms uniforms;
        int viewport_width;
        int viewport_height;

        /* update derived matrices (MVP, normal matrix) */
        void update_matrices();

        /* transform from NDC [-1,1] to screen coordinates */
        Vec3 ndc_to_screen(Vec3 ndc);

    public:
        /* constructor */
        VertexProcessor();

        /* set transformation matrices */
        void set_model_matrix(Mat4 model);
        void set_view_matrix(Mat4 view);
        void set_projection_matrix(Mat4 projection);

        /* convenience: set view and projection from camera */
        void set_camera(Camera& camera);

        /* set viewport dimensions for screen transform */
        void set_viewport(int width, int height);

        /* process a single vertex */
        VertexOutput process_vertex(const VertexInput& input);

        /* get current uniforms */
        Uniforms get_uniforms();
};
