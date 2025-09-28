#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "material.hpp"

struct Triangle {
    glm::vec3 v0, v1, v2; // Vertex positions
    glm::vec3 n0, n1, n2; // Normals (optional, for shading)
};

struct Mesh {
    std::vector<Triangle> triangles;
    Material material;
    std::string id;
};