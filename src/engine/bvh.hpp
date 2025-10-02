#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../renderer/mesh.hpp"

struct BVHNode
{
	glm::vec4 aabbMin;
	glm::vec4 aabbMax;
	unsigned int leftNode;
	unsigned int firstTriIdx;
	unsigned int triCount;
	bool isLeaf() { return triCount > 0; }
};