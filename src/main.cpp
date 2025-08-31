#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.hpp"
#include "renderer/renderer.hpp"

double previousTime = glfwGetTime();
int frameCount = 0;

const unsigned int SCREEN_WIDTH = 1600;
const unsigned int SCREEN_HEIGHT = 900;

int main()
{
    Engine engine(SCREEN_WIDTH, SCREEN_HEIGHT, "Compute Shader Engine");
	Renderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);

    glfwSetCursorPosCallback(engine.m_window, Renderer::mouse_callback);
    glfwSetWindowUserPointer(engine.m_window, &renderer);

	engine.Run(renderer);
    return 0;
}