#pragma once
#include <string>
#include "ui_handler.hpp"

struct Renderer;

class Engine
{
public:
	GLFWwindow* m_window;
	UI_handler* m_ui_handler;

	Engine(int width, int height, std::string title);
	~Engine();

	void Run(Renderer &renderer);

private:
	int m_width;
	int m_height;
	std::string m_title;
	double m_previousTime = glfwGetTime();
	int m_frameCount = 0;

	void InitGLResources();
	void Resize(int width, int height);
	
};