#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <iostream>

#include "../renderer/sphere.hpp"
#include "../renderer/material.hpp"

class Scene
{
public:
	Scene();
	~Scene();
	int AddSphere(const Sphere& s);

	std::vector<Sphere> m_spheres;	
	static constexpr int MAX_SPHERES = 256;
	int m_selectedSphereIndex = -1;
	bool m_spheresDirty = false;
};