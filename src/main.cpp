#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.hpp"
#include "renderer/renderer.hpp"

// ImGui forwarding (needed because we don't let the backend install callbacks)
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

double previousTime = glfwGetTime();
int frameCount = 0;

const unsigned int SCREEN_WIDTH = 1600;
const unsigned int SCREEN_HEIGHT = 900;

// Forward GLFW events to ImGui backend
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    // Optionally: your own mouse-button logic here if (!ImGui::GetIO().WantCaptureMouse) { ... }
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Forward to ImGui first so it can consume the wheel
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    // Then your renderer’s scroll handler (it already skips when ImGui wants the mouse)
    Renderer::scroll_callback(window, xoffset, yoffset);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    // Optionally: your own keyboard handling here if (!ImGui::GetIO().WantCaptureKeyboard) { ... }
}

static void CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

int main()
{
    Engine engine(SCREEN_WIDTH, SCREEN_HEIGHT, "Compute Shader Engine");
    Renderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);

    glfwSetWindowUserPointer(engine.m_window, &renderer);
    glfwMakeContextCurrent(engine.m_window);

    // Mouse move for camera (Renderer handles WantCaptureMouse)
    glfwSetCursorPosCallback(engine.m_window, Renderer::mouse_callback);

    // Forward the rest to ImGui + your handlers
    glfwSetMouseButtonCallback(engine.m_window, MouseButtonCallback);
    glfwSetScrollCallback(engine.m_window, ScrollCallback);
    glfwSetKeyCallback(engine.m_window, KeyCallback);
    glfwSetCharCallback(engine.m_window, CharCallback);

    // Allow interacting with ImGui
    glfwSetInputMode(engine.m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    engine.Run(renderer);
    return 0;
}