#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

enum MaterialType {
	DIFFUSE = 0,
	METAL = 1,
	DIELECTRIC = 2
};

class Material
{
public:
	MaterialType type;
	glm::vec3 albedo;
	float roughness;
	float ior;
	glm::vec2 padding;

	//Material(MaterialType type = DIFFUSE, glm::vec3 albedo = glm::vec3(0.5f), float roughness = 0.0f, float ior = 1.5f);
	Material();
};

//Material::Material(MaterialType type, glm::vec3 albedo, float roughness, float ior)
//	: type(type), albedo(albedo), roughness(roughness), ior(ior), padding(glm::vec2(0.0f))
//{
//}
