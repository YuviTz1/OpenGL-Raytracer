#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class camera {
public:
    glm::vec3 lookfrom;
    glm::vec3 lookat;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 right;
    float fov;
    void updateVectors();
    camera(float fov = 90.0f);
};