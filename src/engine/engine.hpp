#pragma once
#include <string>
#include "ui_handler.hpp"

struct Renderer;

class Engine
{
public:
	GLFWwindow* m_window;
	UI_handler* m_ui_handler;

	Engine(int renderWidth, int renderHeight, std::string title);
	~Engine();

	void Run(Renderer &renderer);

private:
	// Fixed internal render (compute texture) size
	int m_renderWidth;
	int m_renderHeight;

	// Actual window (monitor) size
	int m_windowWidth;
	int m_windowHeight;

	std::string m_title;
	double m_previousTime = glfwGetTime();
	int m_frameCount = 0;

	void InitGLResources();
	void Resize(int width, int height); // (unused now, kept if you extend later)
};