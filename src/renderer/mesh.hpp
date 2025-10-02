#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "material.hpp"

struct Triangle {
    glm::vec3 v0, v1, v2; // Vertex positions
    glm::vec3 n0, n1, n2; // Normals
	glm::vec3 centroid; // Centroid for BVH
};

struct Mesh {
    Material material;
    int numTriangles;
    int startIndex; //triangles buffer start index
	int endIndex;   //triangles buffer end index
    std::string id;
};