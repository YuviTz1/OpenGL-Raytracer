#include "material.hpp"

Material::Material()
	: type(DIFFUSE), albedo(glm::vec4(0.5f)), emission(glm::vec4(0.0f)), roughness(0.0f), ior(1.5f)
{
}