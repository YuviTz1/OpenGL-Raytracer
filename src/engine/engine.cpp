#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 
#include <iostream>
#include <chrono>
#include <thread>
#include "engine.hpp"
#include "../renderer/camera.hpp"
#include "../renderer/renderer.hpp"

Engine::Engine(int renderWidth, int renderHeight, std::string title)
	: m_renderWidth(renderWidth), m_renderHeight(renderHeight), m_title(title),
	  m_windowWidth(renderWidth), m_windowHeight(renderHeight)
{
	m_ui_handler = new UI_handler();
	InitGLResources();
}

Engine::~Engine()
{
	glfwTerminate();
}

void Engine::InitGLResources()
{
	if (!glfwInit())
	{
		std::cout << "Failed to init GLFW\n";
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);     // Borderless
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);     // Fixed size (monitor)
	glfwSwapInterval(0);

	// Query primary monitor size
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	if (mode)
	{
		m_windowWidth = mode->width;
		m_windowHeight = mode->height;
	}
	else
	{
		std::cout << "Failed to get monitor video mode, falling back to render size.\n";
		m_windowWidth = m_renderWidth;
		m_windowHeight = m_renderHeight;
	}

	GLFWwindow* window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_title.c_str(), NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
	{
		std::cout << "GLEW init error" << std::endl;
	}

	// Full window viewport for clears/UI
	glViewport(0, 0, m_windowWidth, m_windowHeight);
	glEnable(GL_DEPTH_TEST);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	m_ui_handler->init(window);
	m_window = window;

	// Place window at (0,0) on the primary monitor (optional)
	//glfwSetWindowPos(window, 0, 0);
}

void Engine::Run(Renderer &renderer)
{
	// Use glfwGetTime consistently for all timing (delta, FPS, frame cap).
	double lastFrameStart = m_previousTime;     // m_previousTime seeded in ctor (initialized with glfwGetTime()).
	double fpsTimerStart = m_previousTime;

	const double targetFrameTime = 1.0 / 60.0;  // 60 FPS cap

	while (!glfwWindowShouldClose(m_window))
	{
		double frameStart = glfwGetTime();
		float deltaTime = static_cast<float>(frameStart - lastFrameStart);
		lastFrameStart = frameStart;

		float aspect = static_cast<float>(m_renderWidth) / static_cast<float>(m_renderHeight);

		// Camera UBO data uses render (compute) resolution aspect
		CameraData cameraData;
		cameraData.position = glm::vec4(renderer.camera.Position, 1.0f);
		cameraData.front = glm::vec4(renderer.camera.Front, 0.0f);
		cameraData.up = glm::vec4(renderer.camera.Up, 0.0f);
		cameraData.right = glm::vec4(renderer.camera.Right, 0.0f);
		cameraData.fovAndAspect = glm::vec2(glm::radians(renderer.camera.Zoom), aspect);
		cameraData.padding = glm::vec2(0.0f);

		// FPS counter (decoupled from per-frame delta)
		m_frameCount++;
		if (frameStart - fpsTimerStart >= 1.0)
		{
			std::cout << "FPS: " << m_frameCount << std::endl;
			renderer.m_frameCount = m_frameCount;
			m_frameCount = 0;
			fpsTimerStart = frameStart;
		}

		glfwPollEvents();

		// Query actual framebuffer size (handles HiDPI)
		int fbW, fbH;
		glfwGetFramebufferSize(m_window, &fbW, &fbH);

		// Clear full window
		glViewport(0, 0, fbW, fbH);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0782f, 0.0782f, 0.0782f, 1.0f);

		renderer.deltaTime = deltaTime;
		renderer.processInput(m_window);

		glBindBuffer(GL_UNIFORM_BUFFER, renderer.m_cameraUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(cameraData), &cameraData);

		// Dispatch compute for the fixed render texture size
		const int localSizeX = 8;
		const int localSizeY = 4;
		const int localSizeZ = 1;
		int groupCountX = (m_renderWidth + localSizeX - 1) / localSizeX;
		int groupCountY = (m_renderHeight + localSizeY - 1) / localSizeY;
		renderer.m_computeShader.use_compute(groupCountX, groupCountY, 1);

		// Center horizontally, align to top vertically.
		// OpenGL origin is bottom-left, so top alignment => y = fbH - renderHeight.
		int xOffset = (fbW - m_renderWidth) / 2;
		if (xOffset < 0) xOffset = 0;

		int yOffset = fbH - m_renderHeight;
		if (yOffset < 0) yOffset = 0;

		glViewport(xOffset, yOffset, m_renderWidth, m_renderHeight);

		// Dynamic bottom bar height: fills the space between bottom of render viewport and bottom of window.
		float bottomBarHeight = static_cast<float>(yOffset);
		if (bottomBarHeight < 0.0f) bottomBarHeight = 0.0f;

		// Left sidebar spans [0, xOffset)
		m_ui_handler->left_sidebar(deltaTime, &renderer.camera.Zoom, xOffset);
		// Right sidebar spans [xOffset + renderWidth, fbW)
		m_ui_handler->right_sidebar(deltaTime, &renderer.camera.Zoom,
			static_cast<float>(xOffset),
			static_cast<float>(m_renderWidth),
			static_cast<float>(fbW));
		// Bottom bar with extended stats
		m_ui_handler->bottom_bar(renderer.m_frameCount, &renderer.camera.Zoom,
			static_cast<float>(xOffset),
			static_cast<float>(m_renderWidth),
			bottomBarHeight,
			m_renderWidth, m_renderHeight,   // viewport (render target) size
			fbW, fbH,                        // window/framebuffer size
			5, 5,                            // samples per pixel, max bounce (hardcoded)
			groupCountX, groupCountY, localSizeZ);

		renderer.m_QuadShader.use();
		glBindTextureUnit(0, renderer.m_screenTex);
		renderer.m_QuadShader.setInt("screen", 0);
		glBindVertexArray(renderer.m_VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Restore viewport for ImGui overlay
		glViewport(0, 0, fbW, fbH);
		m_ui_handler->render();

		glfwSwapBuffers(m_window);

		// Frame limiting (sleep based on glfw time)
		double frameEnd = glfwGetTime();
		double frameDuration = frameEnd - frameStart;
		if (frameDuration < targetFrameTime)
		{
			double sleepSeconds = targetFrameTime - frameDuration;
			std::this_thread::sleep_for(std::chrono::duration<double>(sleepSeconds));
		}

		// Persist last frame start in member (in case Run() is ever re-entered)
		m_previousTime = lastFrameStart;
	}
}

void Engine::Resize(int width, int height)
{
	// (Unused in this scenario; kept for future windowed-resize logic)
}