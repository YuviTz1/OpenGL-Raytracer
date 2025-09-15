#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "material.hpp"

class Sphere
{
public:
	glm::vec4 position;
	float radius;
	Material material;
	Sphere(glm::vec4 position, float radius, Material material);
	Sphere();
};	