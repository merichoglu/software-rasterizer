#pragma once

#include "math/vector.h"
#include "pipeline/vertex_processor.h"
#include <vector>
#include <string>

/* mesh data structure */
struct Mesh {
    std::vector<VertexInput> vertices;
    std::vector<unsigned int> indices;
    std::string name;

    /* get triangle count */
    size_t triangle_count() const {
        return indices.size() / 3;
    }
};

/* model containing one or more meshes */
struct Model {
    std::vector<Mesh> meshes;
    std::string name;

    /* get total triangle count across all meshes */
    size_t triangle_count() const {
        size_t count = 0;
        for (const auto& mesh : meshes) {
            count += mesh.triangle_count();
        }
        return count;
    }
};

class ModelLoader {
    public:
        /* load OBJ file, returns true on success */
        static bool load_obj(const std::string& filepath, Model& model);

        /* compute flat normals for a mesh (per-face normals) */
        static void compute_flat_normals(Mesh& mesh);

        /* compute smooth normals for a mesh (averaged vertex normals) */
        static void compute_smooth_normals(Mesh& mesh);
};
