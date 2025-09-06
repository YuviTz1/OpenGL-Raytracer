#include "sphere.hpp"

Sphere::Sphere()
	: position(glm::vec3(0.0f)), radius(1.0f), material(Material())
{
}

Sphere::Sphere(glm::vec3 position, float radius, Material material)
	: position(position), radius(radius), material(material)
{
}