#include "material.hpp"

Material::Material()
	: type(DIFFUSE), albedo(glm::vec3(0.5f)), roughness(0.0f), ior(1.5f), padding(glm::vec2(0.0f))
{
}