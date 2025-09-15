#include "sphere.hpp"

Sphere::Sphere()
	: position(glm::vec4(0.0f)), radius(1.0f), material(Material())
{
}

Sphere::Sphere(glm::vec4 position, float radius, Material material)
	: position(position), radius(radius), material(material)
{
}