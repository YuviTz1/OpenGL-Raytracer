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

	// New: Left sidebar panel
	void left_sidebar(float deltaTime, float* zoom, float sidebarWidth = 320.0f);

	// Right sidebar: spans from renderStartX + renderWidth to windowWidth
	// Must be called AFTER left_sidebar in the same frame (does not call NewFrame()).
	void right_sidebar(float deltaTime, float* zoom,
		float renderStartX, float renderWidth, float windowWidth);

	//bottom bar: spans between left / right sidebars across render area.
		void bottom_bar(float deltaTime, float* zoom,
			float renderStartX, float renderWidth, float bottomBarHeight);
};