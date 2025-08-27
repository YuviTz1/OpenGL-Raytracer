#pragma once

#include <glm/glm.hpp>

class camera {
public:
    glm::vec3 lookfrom;
    glm::vec3 lookat;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 right;
    float fov;

    camera(float fov = 90.0f)
        : lookfrom(0.0f, 0.0f, 10.0f),
          lookat(0.0f, 0.0f, 0.0f),
          up(0.0f, 1.0f, 0.0f),
          fov(fov)
    {
        updateVectors();
    }

    void updateVectors() {
        forward = glm::normalize(lookat - lookfrom);
        right = glm::normalize(glm::cross(forward, up));
        up = glm::normalize(glm::cross(right, forward));
    }
};