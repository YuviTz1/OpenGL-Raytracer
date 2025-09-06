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

void UI_handler::left_sidebar(float deltaTime, float* zoom, float sidebarWidth)
{
	// Begin a new ImGui frame (ensure you do NOT also call example_ui this frame)
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	// Full-height sidebar from (0,0) to (sidebarWidth, window_height)
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(sidebarWidth, io.DisplaySize.y), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("Sidebar", nullptr, flags);

	ImGui::Text("Renderer Panel");
	ImGui::Separator();
	ImGui::Text("FPS: %.1f", 1.0f / std::max(0.0001f, deltaTime));
	ImGui::SliderFloat("FOV", zoom, 20.0f, 90.0f);

	// Additional controls can go here

	ImGui::End();
}

void UI_handler::right_sidebar(float deltaTime, float* zoom,
	float renderStartX, float renderWidth, float windowWidth)
{
	ImGuiIO& io = ImGui::GetIO();
	float startX = renderStartX + renderWidth;
	float availableWidth = windowWidth - startX;
	if (availableWidth <= 0.0f)
		return;

	ImGui::SetNextWindowPos(ImVec2(startX, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(availableWidth, io.DisplaySize.y), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("RightSidebar", nullptr, flags);
	ImGui::Text("Renderer (Right)");
	ImGui::Separator();
	ImGui::Text("FPS: %.1f", 1.0f / std::max(0.0001f, deltaTime));
	ImGui::SliderFloat("FOV (Right)", zoom, 20.0f, 90.0f);
	// Add additional right panel controls here
	ImGui::End();
}

void UI_handler::bottom_bar(float deltaTime, float* zoom,
	float renderStartX, float renderWidth, float bottomBarHeight)
{
	ImGuiIO& io = ImGui::GetIO();
	float barH = std::max(0.0f, bottomBarHeight);
	if (barH <= 0.0f) return;

	float startX = renderStartX;
	float width = std::max(0.0f, renderWidth);
	if (width <= 0.0f) return;

	float posY = io.DisplaySize.y - barH;
	if (posY < 0.0f) posY = 0.0f;

	ImGui::SetNextWindowPos(ImVec2(startX, posY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, barH), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("BottomBar", nullptr, flags);
	ImGui::Text("Bottom Bar");
	ImGui::Separator();
	ImGui::Text("Viewport Width: %.0f", width);
	ImGui::Text("Delta: %.3f ms", 1000.0f * deltaTime);
	ImGui::SliderFloat("FOV (Bottom)", zoom, 20.0f, 90.0f);
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