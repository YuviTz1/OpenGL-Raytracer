#include "camera.hpp"

camera::camera(float fov)
    : lookfrom(0.0f, 0.0f, -10.0f),
    lookat(0.0f, 0.0f, 0.0f),
    up(0.0f, 1.0f, 0.0f),
    fov(fov)
{
    updateVectors();
}

void camera::updateVectors() {
    forward = glm::normalize(lookat - lookfrom);
    right = glm::normalize(glm::cross(forward, up));
    up = glm::normalize(glm::cross(right, forward));
}