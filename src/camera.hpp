#pragma once

#include <glm/glm.hpp>

class camera {
public:
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 forward;
    float fov;

    camera(float fov) : position(0.0f, 0.0f, -tan(fov / 2.0f)), up(0.0f, 1.0f, 0.0f), right(-1.0f, 0.0f, 0.0f), forward(0.0f, 0.0f, -1.0f) {}

};