#include "model_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

bool ModelLoader::load_obj(const std::string& filepath, Model& model) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filepath << std::endl;
        return false;
    }

    /* temporary storage for raw OBJ data */
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texcoords;

    /* current mesh being built */
    Mesh current_mesh;
    current_mesh.name = "default";

    /* map to track unique vertex combinations and reuse indices */
    std::unordered_map<std::string, unsigned int> vertex_map;

    std::string line;
    while (std::getline(file, line)) {
        /* skip empty lines and comments */
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            /* vertex position */
            Vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vn") {
            /* vertex normal */
            Vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "vt") {
            /* texture coordinate */
            Vec2 uv;
            iss >> uv.x >> uv.y;
            texcoords.push_back(uv);
        }
        else if (prefix == "f") {
            /* face - can be triangles or quads, with various formats:
               f v1 v2 v3
               f v1/vt1 v2/vt2 v3/vt3
               f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
               f v1//vn1 v2//vn2 v3//vn3 */

            std::vector<unsigned int> face_indices;
            std::string vertex_str;

            while (iss >> vertex_str) {
                /* check if we've seen this exact vertex combination before */
                auto it = vertex_map.find(vertex_str);
                if (it != vertex_map.end()) {
                    face_indices.push_back(it->second);
                    continue;
                }

                /* parse the vertex string */
                int pos_idx = 0, tex_idx = 0, norm_idx = 0;
                std::replace(vertex_str.begin(), vertex_str.end(), '/', ' ');
                std::istringstream vss(vertex_str);

                vss >> pos_idx;
                if (!vss.eof()) {
                    if (vss.peek() != ' ' || !vss.eof()) {
                        vss >> tex_idx;
                    }
                }
                if (!vss.eof()) {
                    vss >> norm_idx;
                }

                /* OBJ indices are 1-based, convert to 0-based */
                /* negative indices are relative to current position */
                if (pos_idx < 0) {
                    pos_idx = static_cast<int>(positions.size()) + pos_idx + 1;
                }
                if (tex_idx < 0) {
                    tex_idx = static_cast<int>(texcoords.size()) + tex_idx + 1;
                }
                if (norm_idx < 0) {
                    norm_idx = static_cast<int>(normals.size()) + norm_idx + 1;
                }

                /* create the vertex */
                VertexInput vertex;
                vertex.position = positions[pos_idx - 1];
                vertex.normal = (norm_idx > 0 && norm_idx <= static_cast<int>(normals.size()))
                    ? normals[norm_idx - 1] : Vec3(0.0f, 1.0f, 0.0f);
                vertex.tex_coord = (tex_idx > 0 && tex_idx <= static_cast<int>(texcoords.size()))
                    ? texcoords[tex_idx - 1] : Vec2(0.0f, 0.0f);
                vertex.color = Color(1.0f, 1.0f, 1.0f, 1.0f);

                /* add vertex and store index */
                unsigned int new_index = static_cast<unsigned int>(current_mesh.vertices.size());
                current_mesh.vertices.push_back(vertex);
                vertex_map[vertex_str] = new_index;
                face_indices.push_back(new_index);
            }

            /* triangulate the face (fan triangulation for convex polygons) */
            for (size_t i = 1; i + 1 < face_indices.size(); ++i) {
                current_mesh.indices.push_back(face_indices[0]);
                current_mesh.indices.push_back(face_indices[i]);
                current_mesh.indices.push_back(face_indices[i + 1]);
            }
        }
        else if (prefix == "o" || prefix == "g") {
            /* object or group name - start a new mesh if current has data */
            if (!current_mesh.vertices.empty()) {
                model.meshes.push_back(current_mesh);
                current_mesh = Mesh();
                vertex_map.clear();
            }
            iss >> current_mesh.name;
        }
    }

    /* add the last mesh */
    if (!current_mesh.vertices.empty()) {
        model.meshes.push_back(current_mesh);
    }

    file.close();

    /* extract model name from filepath */
    size_t last_slash = filepath.find_last_of("/\\");
    size_t last_dot = filepath.find_last_of('.');
    if (last_slash == std::string::npos) {
        last_slash = 0;
    } else {
        last_slash++;
    }
    model.name = filepath.substr(last_slash, last_dot - last_slash);

    std::cout << "Loaded model: " << model.name << std::endl;
    std::cout << "  Meshes: " << model.meshes.size() << std::endl;
    std::cout << "  Total triangles: " << model.triangle_count() << std::endl;

    return true;
}

void ModelLoader::compute_flat_normals(Mesh& mesh) {
    /* for flat shading, each triangle needs its own vertices with face normal */
    std::vector<VertexInput> new_vertices;
    std::vector<unsigned int> new_indices;

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        VertexInput v0 = mesh.vertices[mesh.indices[i]];
        VertexInput v1 = mesh.vertices[mesh.indices[i + 1]];
        VertexInput v2 = mesh.vertices[mesh.indices[i + 2]];

        /* compute face normal */
        Vec3 edge1 = v1.position - v0.position;
        Vec3 edge2 = v2.position - v0.position;
        Vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        /* assign face normal to all three vertices */
        v0.normal = normal;
        v1.normal = normal;
        v2.normal = normal;

        /* add as new vertices */
        unsigned int base_idx = static_cast<unsigned int>(new_vertices.size());
        new_vertices.push_back(v0);
        new_vertices.push_back(v1);
        new_vertices.push_back(v2);

        new_indices.push_back(base_idx);
        new_indices.push_back(base_idx + 1);
        new_indices.push_back(base_idx + 2);
    }

    mesh.vertices = std::move(new_vertices);
    mesh.indices = std::move(new_indices);
}

void ModelLoader::compute_smooth_normals(Mesh& mesh) {
    /* reset all normals to zero */
    for (auto& vertex : mesh.vertices) {
        vertex.normal = Vec3(0.0f);
    }

    /* accumulate face normals at each vertex */
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        unsigned int i0 = mesh.indices[i];
        unsigned int i1 = mesh.indices[i + 1];
        unsigned int i2 = mesh.indices[i + 2];

        Vec3 p0 = mesh.vertices[i0].position;
        Vec3 p1 = mesh.vertices[i1].position;
        Vec3 p2 = mesh.vertices[i2].position;

        /* compute face normal */
        Vec3 edge1 = p1 - p0;
        Vec3 edge2 = p2 - p0;
        Vec3 normal = glm::cross(edge1, edge2);

        /* accumulate (weighted by face area, which is magnitude of cross product) */
        mesh.vertices[i0].normal += normal;
        mesh.vertices[i1].normal += normal;
        mesh.vertices[i2].normal += normal;
    }

    /* normalize all vertex normals */
    for (auto& vertex : mesh.vertices) {
        if (glm::length(vertex.normal) > 0.0001f) {
            vertex.normal = glm::normalize(vertex.normal);
        } else {
            vertex.normal = Vec3(0.0f, 1.0f, 0.0f);
        }
    }
}
