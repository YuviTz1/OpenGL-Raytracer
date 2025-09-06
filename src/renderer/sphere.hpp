#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include "material.hpp"

class Sphere
{
public:
	glm::vec3 position;
	float radius;
	Material material;
	Sphere(glm::vec3 position, float radius, Material material);
	Sphere();
};	