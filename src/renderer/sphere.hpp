#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include "material.hpp"

struct Sphere
{
	glm::vec3 position;
	float radius;
	Material material;
};