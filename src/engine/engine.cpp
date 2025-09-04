#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 
#include <iostream>
#include <chrono>
#include <thread>
#include "engine.hpp"
#include "../renderer/camera.hpp"
#include "../renderer/renderer.hpp"

// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

Engine::Engine(int width, int height, std::string title)
	: m_width(width), m_height(height), m_title(title)
{
	InitGLResources();
}

Engine::~Engine()
{
    // ImGui shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void Engine::InitGLResources()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(0);

    GLFWwindow* window;
    window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // check if GLEW is working fine
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW init error" << std::endl;
    }

    glViewport(0, 0, m_width, m_height);
    glEnable(GL_DEPTH_TEST);

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Use modern GLSL for backend shaders
    const char* glsl_version = "#version 460";
    // Do NOT install callbacks to avoid interfering with your own
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init(glsl_version);

    m_window = window;
}

void Engine::Run(Renderer &renderer)
{
    while(!glfwWindowShouldClose(m_window))
    {
        auto frameStart = std::chrono::high_resolution_clock::now();

        double currentTime = glfwGetTime();
        float deltaTime = currentTime - m_previousTime;
		float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);

        CameraData cameraData;
        cameraData.position = glm::vec4(renderer.camera.Position, 1.0f);
        cameraData.front = glm::vec4(renderer.camera.Front, 0.0f);
        cameraData.up = glm::vec4(renderer.camera.Up, 0.0f);
        cameraData.right = glm::vec4(renderer.camera.Right, 0.0f);
        cameraData.fovAndAspect = glm::vec2(glm::radians(renderer.camera.Zoom), aspect);
        cameraData.padding = glm::vec2(0.0f);

        m_frameCount++;
        if (currentTime - m_previousTime >= 1.0)
        {
            std::cout << "FPS: " << m_frameCount << std::endl;
            m_frameCount = 0;
            m_previousTime = currentTime;
        }

        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Example UI
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", 1.0f / std::max(0.0001f, deltaTime));
        ImGui::SliderFloat("FOV", &renderer.camera.Zoom, 20.0f, 90.0f);
        ImGui::End();

        // clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0782f, 0.0782f, 0.0782f, 1.0f);

		renderer.deltaTime = deltaTime;
        renderer.processInput(m_window);

        glBindBuffer(GL_UNIFORM_BUFFER, renderer.m_cameraUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(cameraData), &cameraData);

        renderer.m_computeShader.use_compute(ceil(m_width / 8), ceil(m_height / 4), 1);
        renderer.m_QuadShader.use();
        glBindTextureUnit(0, renderer.m_screenTex);
        renderer.m_QuadShader.setInt("screen", 0);
        glBindVertexArray(renderer.m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // ImGui render (after your scene)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);

        // Frame limiting to 60 FPS
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> frameDuration = frameEnd - frameStart;
        double targetFrameTime = 1000 / 60.0f;

        if (frameDuration.count() < targetFrameTime)
        {
            std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(targetFrameTime - frameDuration.count())
            );
        }
	}
}