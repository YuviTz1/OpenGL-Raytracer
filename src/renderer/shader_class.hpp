// helper class for shader compilation

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp> 

class Shader
{
public:
	unsigned int ID;

	Shader(const char *vertexPath, const char *fragmentPath);
	Shader(const char *computePath);
	void use();
	void use_compute(int x, int y, int z);

	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setMat4(const std::string &name, glm::mat4 value) const;
};