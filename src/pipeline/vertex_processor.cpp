#include "pipeline/vertex_processor.h"

VertexProcessor::VertexProcessor() {
    uniforms.model_matrix = Mat4(1.0f);
    uniforms.view_matrix = Mat4(1.0f);
    uniforms.projection_matrix = Mat4(1.0f);
    uniforms.mvp_matrix = Mat4(1.0f);
    uniforms.normal_matrix = Mat3(1.0f);
    viewport_width = 800;
    viewport_height = 600;
}

void VertexProcessor::set_model_matrix(Mat4 model) {
    uniforms.model_matrix = model;
    update_matrices();
}

void VertexProcessor::set_view_matrix(Mat4 view) {
    uniforms.view_matrix = view;
    update_matrices();
}

void VertexProcessor::set_projection_matrix(Mat4 projection) {
    uniforms.projection_matrix = projection;
    update_matrices();
}

void VertexProcessor::set_camera(Camera& camera) {
    uniforms.view_matrix = camera.get_view_matrix();
    uniforms.projection_matrix = camera.get_projection_matrix();
    update_matrices();
}

void VertexProcessor::set_viewport(int width, int height) {
    viewport_width = width;
    viewport_height = height;
}

Uniforms VertexProcessor::get_uniforms() {
    return uniforms;
}

void VertexProcessor::update_matrices() {
    /* compute combined MVP matrix */
    uniforms.mvp_matrix = uniforms.projection_matrix * uniforms.view_matrix * uniforms.model_matrix;

    /* compute normal matrix: transpose of inverse of upper-left 3x3 of model matrix */
    Mat3 model_mat3 = Mat3(uniforms.model_matrix);
    uniforms.normal_matrix = glm::transpose(glm::inverse(model_mat3));
}

Vec3 VertexProcessor::ndc_to_screen(Vec3 ndc) {
    /* NDC x, y in [-1, 1], z in [-1, 1] */
    /* Screen x in [0, width], y in [0, height], z in [0, 1] */
    float screen_x = (ndc.x + 1.0f) * 0.5f * viewport_width;
    float screen_y = (1.0f - ndc.y) * 0.5f * viewport_height;  /* flip y */
    float screen_z = (ndc.z + 1.0f) * 0.5f;                     /* map to [0, 1] */

    return Vec3(screen_x, screen_y, screen_z);
}

VertexOutput VertexProcessor::process_vertex(const VertexInput& input) {
    VertexOutput output;

    /* transform position to clip space */
    output.clip_pos = uniforms.mvp_matrix * Vec4(input.position, 1.0f);

    /* perspective divide to get NDC coordinates */
    if (output.clip_pos.w != 0) {
        output.ndc_pos = Vec3(
            output.clip_pos.x / output.clip_pos.w,
            output.clip_pos.y / output.clip_pos.w,
            output.clip_pos.z / output.clip_pos.w
        );
    } else {
        output.ndc_pos = Vec3(output.clip_pos.x, output.clip_pos.y, output.clip_pos.z);
    }

    /* transform NDC to screen coordinates */
    output.screen_pos = ndc_to_screen(output.ndc_pos);

    /* transform position to world space (for lighting calculations) */
    Vec4 world_pos4 = uniforms.model_matrix * Vec4(input.position, 1.0f);
    output.world_pos = Vec3(world_pos4.x, world_pos4.y, world_pos4.z);

    /* transform normal to world space */
    output.normal = glm::normalize(uniforms.normal_matrix * input.normal);

    /* pass through texture coordinates and color */
    output.tex_coord = input.tex_coord;
    output.color = input.color;

    return output;
}
