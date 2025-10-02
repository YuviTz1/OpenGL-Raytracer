#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <iostream>

#include "../renderer/sphere.hpp"
#include "../renderer/material.hpp"
#include "../renderer/mesh.hpp"
#include "bvh.hpp"

class Scene
{
public:
	Scene();
	~Scene();
	int AddSphere(const Sphere& s);
	bool SaveToFile(const std::string& path);
	bool LoadFromFile(const std::string& path, bool& shouldResetAccumulation);

	bool LoadOBJ(const std::string& filename, Mesh& mesh, bool& shouldResetAccumulation);
	bool RemoveMesh(int index, bool& shouldResetAccumulation); // NEW

	std::vector<Sphere> m_spheres;	
	std::vector<Mesh> m_meshes;
	std::vector<Triangle> m_triangles; // all triangles from all meshes
	static constexpr int MAX_SPHERES = 256;
	static constexpr int MAX_MESHES = 256;
	static constexpr int MAX_TRIANGLES = 1024; //todo: check for num triangles
	std::vector<BVHNode> m_bvh;
	std::vector<unsigned int> m_triangleIndices;
	unsigned int m_rootNodeIdx = 0, m_nodesUsed = 1;
	int m_selectedSphereIndex = -1;
	int m_selectedMeshIndex = -1;
	bool m_spheresDirty = false;
	bool m_meshesDirty = false;

	void BuildBVH();
	void UpdateNodeBounds(unsigned int nodeIdx);
	void SubDivide(unsigned int nodeIdx);
};