#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

enum MaterialType {
	DIFFUSE = 0,
	METAL = 1,
	DIELECTRIC = 2
};

class Material
{
public:
	MaterialType type;
	glm::vec4 albedo;
	float roughness;
	float ior;

	//Material(MaterialType type = DIFFUSE, glm::vec3 albedo = glm::vec3(0.5f), float roughness = 0.0f, float ior = 1.5f);
	Material();
};
