#include "ui_handler.hpp"
#include <algorithm>
#include <cstring>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "../renderer/camera.hpp"
#include "../renderer/renderer.hpp"
#include "scene.hpp" // Add this include

void UI_handler::render()
{
	// ImGui render (after your scene)
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI_handler::left_sidebar(float deltaTime, float sidebarWidth)
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

	ImGui::Text("Scene");
	ImGui::Separator();

	if (m_spheres)
	{
		ImGui::Separator();
		if (ImGui::Button("Add Sphere"))
		{
			Sphere s;
			s.position = glm::vec4(0.0f, 0.0f, -6.0f, 0.0f);
			s.radius = 1.0f;
			s.material.type = DIFFUSE;
			s.material.albedo = glm::vec4(0.7f, 0.3f, 0.3f, 0.0f);
			m_spheres->push_back(s);
			if (m_selectedSphere) *m_selectedSphere = (int)m_spheres->size() - 1;
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		if (!m_spheres->empty())
		{
			if (ImGui::Button("Delete Selected") && m_selectedSphere && *m_selectedSphere >= 0)
			{
				int idx = *m_selectedSphere;
				if (idx >= 0 && idx < (int)m_spheres->size())
				{
					m_spheres->erase(m_spheres->begin() + idx);
					if (m_spheresDirty) *m_spheresDirty = true;
					if (m_resetAccumulation) *m_resetAccumulation = true;
					if (m_spheres->empty()) *m_selectedSphere = -1;
					else *m_selectedSphere = std::min(idx, (int)m_spheres->size() - 1);
				}
			}
			ImGui::Separator();
			for (int i = 0; i < (int)m_spheres->size(); ++i)
			{
				char label[32];
				//snprintf(label, 32, "Sphere %d", i);
				std::snprintf(label, 32, "%s##%d", (*m_spheres)[i].id.c_str(), i); // use id as label, but ensure unique with ##
				bool sel = (m_selectedSphere && *m_selectedSphere == i);
				if (ImGui::Selectable(label, sel))
				{
					if (m_selectedSphere) *m_selectedSphere = i;
					if (m_resetAccumulation) *m_resetAccumulation = true;
				}
			}
		}
	}
	

	// Additional controls can go here

	ImGui::End();
}

void UI_handler::right_sidebar(float renderStartX, float renderWidth, float windowWidth)
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
	ImGui::Text("Properties");
	ImGui::Separator();

	// Selected sphere inspector
	if (m_spheres && m_selectedSphere && *m_selectedSphere >= 0 && *m_selectedSphere < (int)m_spheres->size())
	{
		int idx = *m_selectedSphere;
		Sphere& s = (*m_spheres)[idx];

		ImGui::Text("Selected Sphere: %s", s.id.c_str());
		ImGui::Separator();

		// Name / ID (editable)
		{
			char nameBuf[32] = {0};
			// copy current id into buffer (truncate if longer than buffer)
			std::strncpy(nameBuf, s.id.c_str(), sizeof(nameBuf) - 1);
			if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
			{
				s.id = std::string(nameBuf);
				if (m_spheresDirty) *m_spheresDirty = true;
				if (m_resetAccumulation) *m_resetAccumulation = true;
			}
		}

		// Position (editable)
		float pos[3] = { s.position.x, s.position.y, s.position.z };
		if (ImGui::DragFloat3("Position", pos, 0.05f, -1000.0f, 1000.0f, "%.3f"))
		{
			s.position.x = pos[0];
			s.position.y = pos[1];
			s.position.z = pos[2];
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		// Radius
		if (ImGui::DragFloat("Radius", &s.radius, 0.05f, -1000.0f, 1000.0f, "%.3f"))
		{
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		ImGui::Separator();
		ImGui::Text("Material");
		// Material type
		const char* materialItems[] = { "Diffuse", "Metal", "Dielectric" };
		int matType = (int)s.material.type;
		if (ImGui::Combo("Type", &matType, materialItems, IM_ARRAYSIZE(materialItems)))
		{
			s.material.type = (MaterialType)matType;
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		// Albedo (color)
		float albedo[3] = { s.material.albedo.x, s.material.albedo.y, s.material.albedo.z };
		if (ImGui::ColorEdit3("Albedo", albedo))
		{
			s.material.albedo.x = albedo[0];
			s.material.albedo.y = albedo[1];
			s.material.albedo.z = albedo[2];
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}
		// expose alpha if needed
		if (ImGui::InputFloat("Albedo A", &s.material.albedo.w, 0.0f, 0.0f, "%.3f"))
		{
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		// Roughness and IOR
		if (ImGui::SliderFloat("Roughness", &s.material.roughness, 0.0f, 1.0f))
		{
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}
		if (ImGui::SliderFloat("IOR", &s.material.ior, 1.0f, 3.0f))
		{
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		ImGui::Separator();
	}
	else
	{
		ImGui::Text("No sphere selected");
		ImGui::Separator();
	}
	ImGui::End();
}

void UI_handler::bottom_bar(float fps, float* zoom,
	float renderStartX, float renderWidth, float bottomBarHeight,
	int viewportWidth, int viewportHeight,
	int windowWidth, int windowHeight,
	int samplesPerPixel, int maxBounce,
	int localSizeX, int localSizeY, int localSizeZ)
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
	ImGui::Text("Render Stats");
	ImGui::Separator();

	float clampedFPS = std::max(0.00001f, fps);
	float frameTimeMs = 1000.0f / clampedFPS;

	ImGui::Text("Viewport: %d x %d", viewportWidth, viewportHeight);
	ImGui::Text("Window:   %d x %d", windowWidth, windowHeight);
	ImGui::Text("Frame: %.2f ms (%.1f FPS)", frameTimeMs, clampedFPS);
	ImGui::Text("Samples Per Pixel: %d  | Max Bounce: %d", samplesPerPixel, maxBounce);
	ImGui::Text("Work Group Size: %d x %d x %d", localSizeX, localSizeY, localSizeZ);

	// Added code for scene save/load
	static char savePath[256] = "scene_save.bin";
	static char loadPath[256] = "scene_save.bin";
	static char statusMsg[128] = "";

	ImGui::Separator();
	ImGui::Text("Scene Save/Load");

	ImGui::InputText("Save Path", savePath, sizeof(savePath));
	if (ImGui::Button("Save Scene")) {
		if (m_scene) {
			if (m_scene->SaveToFile(savePath))
				std::snprintf(statusMsg, sizeof(statusMsg), "Scene saved to %s", savePath);
			else
				std::snprintf(statusMsg, sizeof(statusMsg), "Failed to save scene!");
		}
	}

	ImGui::InputText("Load Path", loadPath, sizeof(loadPath));
	if (ImGui::Button("Load Scene")) {
		if (m_scene && m_resetAccumulation_external) {
			bool reset = false;
			if (m_scene->LoadFromFile(loadPath, reset)) {
				*m_resetAccumulation_external = true;
				std::snprintf(statusMsg, sizeof(statusMsg), "Scene loaded from %s", loadPath);
			} else {
				std::snprintf(statusMsg, sizeof(statusMsg), "Failed to load scene!");
			}
		}
	}

	ImGui::Text("%s", statusMsg);

	ImGui::End();
}

void UI_handler::bindSpheres(std::vector<Sphere>* spheres, int* selectedIndex, bool* spheresDirty, bool* resetAccumulation)
{
	m_spheres = spheres;
	m_selectedSphere = selectedIndex;
	m_spheresDirty = spheresDirty;
	m_resetAccumulation = resetAccumulation;
}

void UI_handler::setScene(Scene* scene, bool* resetAccumulation)
{
	m_scene = scene;
	m_resetAccumulation_external = resetAccumulation;
}

UI_handler::UI_handler()
{
	// ImGui init
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	io.FontGlobalScale = 1.7f;
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