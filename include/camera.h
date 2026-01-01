#pragma once

#include "math/matrix.h"
#include "math/vector.h"

class Camera {
    private:
        Vec3 position;
        Vec3 target;
        Vec3 up;

        float fov;
        float aspect_ratio;
        float near_plane;
        float far_plane;

        Mat4 view_matrix;
        Mat4 proj_matrix;
        Mat4 view_proj_matrix;

        bool dirty;

        /* recalculate matrices if dirty flag is set */
        void update_matrices();

    public:
        /* constructor */
        Camera();

        /* setters, mark dirty when changed */
        void set_position(Vec3 i_position);
        void set_target(Vec3 i_target);
        void set_up(Vec3 i_up);
        void set_fov(float i_fov);
        void set_aspect_ratio(float i_aspect_ratio);
        void set_near_plane(float i_near_plane);
        void set_far_plane(float i_far_plane);

        /* configure perspective projection */
        void set_perspective(float i_fov, float i_aspect_ratio, float i_near_plane, float i_far_plane);

        /* set view using look_at parameters */
        void look_at(Vec3 eye, Vec3 center, Vec3 i_up);

        /* getters */
        Vec3 get_position();
        Vec3 get_target();
        Vec3 get_forward();
        Vec3 get_right();
        Vec3 get_up();

        /* matrix getters, recalculate if dirty */
        Mat4 get_view_matrix();
        Mat4 get_projection_matrix();
        Mat4 get_view_projection_matrix();
};