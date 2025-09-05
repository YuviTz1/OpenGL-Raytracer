#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Forward declaration instead of including the full camera header
class Camera;

class UI_handler
{
public:
	UI_handler();
	~UI_handler();
	void render();
	void example_ui(float deltaTime, float* zoom);
	void init(GLFWwindow* window);
};