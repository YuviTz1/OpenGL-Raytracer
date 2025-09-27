#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include <vector>
#include "../renderer/sphere.hpp"

class Scene; // forward declare

class UI_handler
{
public:
	UI_handler();
	~UI_handler();
	void render();
	void init(GLFWwindow* window);

	// New: Left sidebar panel
	void left_sidebar(float deltaTime, float sidebarWidth = 320.0f);

	// Right sidebar: spans from renderStartX + renderWidth to windowWidth
	// Must be called AFTER left_sidebar in the same frame (does not call NewFrame()).
	void right_sidebar(float renderStartX, float renderWidth, float windowWidth);

	// Bottom bar: with render stats
	void bottom_bar(float fps, float* zoom,
		float renderStartX, float renderWidth, float bottomBarHeight,
		int viewportWidth, int viewportHeight,
		int windowWidth, int windowHeight,
		int samplesPerPixel, int maxBounce,
		int localSizeX, int localSizeY, int localSizeZ);

	// New binding
	void bindSpheres(std::vector<Sphere>* spheres,
		int* selectedIndex,
		bool* spheresDirty,
		bool* resetAccumulation);

	void setScene(Scene* scene, bool* resetAccumulation, float* backgroundStrength);

private:
	std::vector<Sphere>* m_spheres = NULL;
	int* m_selectedSphere = NULL;
	bool* m_spheresDirty = NULL;
	bool* m_resetAccumulation = NULL;
	Scene* m_scene = NULL;
	bool* m_resetAccumulation_external = NULL;
	float* m_backgroundStrength = NULL;
};