#include "ui_handler.hpp"
#include <algorithm>

void UI_handler::render()
{
	// ImGui render (after your scene)
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI_handler::example_ui(float deltaTime, float* zoom)
{
	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Example UI
	ImGui::Begin("Stats");
	ImGui::Text("FPS: %.1f", 1.0f / std::max(0.0001f, deltaTime));
	ImGui::SliderFloat("FOV", zoom, 20.0f, 90.0f);
	ImGui::End();
}

UI_handler::UI_handler()
{
	// ImGui init
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
}

void UI_handler::init(GLFWwindow* window)
{
	// Use modern GLSL for backend shaders
	const char* glsl_version = "#version 460";
	// Do NOT install callbacks to avoid interfering with your own
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

UI_handler::~UI_handler()
{
	// ImGui shutdown
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}