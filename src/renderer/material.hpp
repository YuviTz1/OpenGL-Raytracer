#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class Material
{
public:
	int type;
	glm::vec3 albedo;
	float roughness;
	float ior;
	glm::vec2 padding;
};