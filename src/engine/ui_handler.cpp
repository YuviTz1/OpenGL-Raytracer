#include "ui_handler.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

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

	// Split the left sidebar into two vertical children:
	// - Top half: Combined list (spheres + meshes) and sphere actions
	// - Bottom half: .obj browser in res with Load buttons
	const float availY = ImGui::GetContentRegionAvail().y;
	const float topH   = std::max(0.0f, availY * 0.5f);
	const float bottomH = std::max(0.0f, availY - topH);

	// ---------------- Top Half ----------------
	ImGui::BeginChild("TopHalf", ImVec2(0.0f, topH), false);

	ImGui::Text("Scene Objects");
	ImGui::Separator();

	const int sphereCount = (m_spheres ? (int)m_spheres->size() : 0);
	const int meshCount   = (m_meshes ? (int)m_meshes->size() : 0);

	// Compute combined selection index for UI
	int combinedSelected = -1;
	if (m_selectedSphere && *m_selectedSphere >= 0) {
		combinedSelected = *m_selectedSphere; // spheres first
	} else if (m_selectedMesh && *m_selectedMesh >= 0) {
		combinedSelected = sphereCount + *m_selectedMesh;
	}

	// List spheres
	for (int i = 0; i < sphereCount; ++i)
	{
		const Sphere& s = (*m_spheres)[i];
		char label[128];
		std::snprintf(label, sizeof(label), "Sphere: %s##sphere_%d", s.id.c_str(), i);
		bool sel = (combinedSelected == i);
		if (ImGui::Selectable(label, sel))
		{
			if (m_selectedSphere) *m_selectedSphere = i;
			if (m_selectedMesh) *m_selectedMesh = -1;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}
	}

	// List meshes with triangle counts
	for (int i = 0; i < meshCount; ++i)
	{
		const Mesh& m = (*m_meshes)[i];
		char label[160];
		std::snprintf(label, sizeof(label), "Mesh: %s (%d tris)##mesh_%d", m.id.c_str(), m.numTriangles, i);
		bool sel = (combinedSelected == (sphereCount + i));
		if (ImGui::Selectable(label, sel))
		{
			if (m_selectedSphere) *m_selectedSphere = -1;
			if (m_selectedMesh) *m_selectedMesh = i;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}
	}

	ImGui::Separator();

	// Sphere actions
	if (m_spheres)
	{
		if (ImGui::Button("Add Sphere"))
		{
			Sphere s;
			s.id = "Sphere";
			s.position = glm::vec4(0.0f, 0.0f, -6.0f, 0.0f);
			s.radius = 1.0f;
			s.material.type = DIFFUSE;
			s.material.albedo = glm::vec4(0.7f, 0.3f, 0.3f, 0.0f);
			m_spheres->push_back(s);
			if (m_selectedSphere) *m_selectedSphere = (int)m_spheres->size() - 1;
			if (m_selectedMesh) *m_selectedMesh = -1;
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		// Delete only if a sphere is selected
		if (m_selectedSphere && *m_selectedSphere >= 0 && !m_spheres->empty())
		{
			ImGui::SameLine();
			if (ImGui::Button("Delete Sphere"))
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
		}
	}

	// Mesh actions
	if (m_selectedMesh && *m_selectedMesh >= 0 && m_meshes && !m_meshes->empty())
	{
		if (ImGui::Button("Delete Mesh"))
		{
			if (m_scene)
			{
				bool shouldReset = false;
				const int idx = *m_selectedMesh;
				if (m_scene->RemoveMesh(idx, shouldReset))
				{
					// Selection is updated inside Scene::RemoveMesh (m_selectedMeshIndex)
					if (m_resetAccumulation_external && shouldReset)
						*m_resetAccumulation_external = true;
				}
			}
		}
	}

	ImGui::EndChild(); // TopHalf

	// ---------------- Bottom Half ----------------
	ImGui::BeginChild("BottomHalf", ImVec2(0.0f, bottomH), false);

	ImGui::Text("OBJ Browser");
	ImGui::Separator();

	// Folder input (defaults to "res")
	static char objFolder[260] = "res";
	ImGui::InputText("Folder", objFolder, sizeof(objFolder));

	// Discover .obj files in folder each frame (simple, fine for small folders)
	std::vector<std::filesystem::path> objFiles;
	{
		std::error_code ec;
		const std::filesystem::path root = std::filesystem::path(objFolder);
		if (std::filesystem::exists(root, ec) && std::filesystem::is_directory(root, ec))
		{
			for (auto it = std::filesystem::directory_iterator(root, ec); !ec && it != std::filesystem::end(it); it.increment(ec))
			{
				const auto& p = it->path();
				if (!it->is_regular_file(ec)) continue;
				auto ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
				if (ext == ".obj") objFiles.push_back(p);
			}
			std::sort(objFiles.begin(), objFiles.end(), [](const auto& a, const auto& b) { return a.filename().string() < b.filename().string(); });
		}
	}

	static char loadStatus[128] = "";
	if (objFiles.empty())
	{
		ImGui::TextDisabled("No .obj files found");
	}
	else
	{
		// Scrollable list
		if (ImGui::BeginChild("OBJList", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), true))
		{
			for (int i = 0; i < (int)objFiles.size(); ++i)
			{
				const auto& p = objFiles[i];
				const std::string fname = p.filename().string();

				ImGui::Text("%s", fname.c_str());
				ImGui::SameLine();
				char btnId[64];
				std::snprintf(btnId, sizeof(btnId), "Load##obj_%d", i);
				if (ImGui::Button(btnId))
				{
					if (m_scene)
					{
						Mesh mesh{};
						mesh.id = p.stem().string();
						bool shouldReset = false;
						const std::string full = p.string();

						if (m_scene->LoadOBJ(full, mesh, shouldReset))
						{
							std::snprintf(loadStatus, sizeof(loadStatus), "Loaded: %s", fname.c_str());
							// Select the newly loaded mesh
							if (m_selectedSphere) *m_selectedSphere = -1;
							if (m_selectedMesh && m_meshes) *m_selectedMesh = (int)m_meshes->size() - 1;
							if (m_resetAccumulation_external && shouldReset) *m_resetAccumulation_external = true;
						}
						else
						{
							std::snprintf(loadStatus, sizeof(loadStatus), "Failed to load: %s", fname.c_str());
						}
					}
					else
					{
						std::snprintf(loadStatus, sizeof(loadStatus), "No active scene to load into");
					}
				}
			}
			ImGui::EndChild();
		}

		ImGui::Text("%s", loadStatus);
	}

	ImGui::EndChild(); // BottomHalf

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

	const bool hasSphereSel = (m_spheres && m_selectedSphere && *m_selectedSphere >= 0 && *m_selectedSphere < (int)m_spheres->size());
	const bool hasMeshSel   = (m_meshes  && m_selectedMesh   && *m_selectedMesh   >= 0 && *m_selectedMesh   < (int)m_meshes->size());

	// Sphere inspector
	if (hasSphereSel)
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
		const char* materialItems[] = { "Diffuse", "Metal", "Dielectric" , "Light"};
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
		/*if (ImGui::InputFloat("Albedo A", &s.material.albedo.w, 0.0f, 0.0f, "%.3f"))
		{
			if (m_spheresDirty) *m_spheresDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}*/

		//emission
		float emission[3] = { s.material.emission.x, s.material.emission.y, s.material.emission.z };
		if (ImGui::DragFloat3("Emission", emission))
		{
			s.material.emission.x = emission[0];
			s.material.emission.y = emission[1];
			s.material.emission.z = emission[2];
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
	// Mesh inspector
	else if (hasMeshSel)
	{
		int idx = *m_selectedMesh;
		Mesh& m = (*m_meshes)[idx];

		ImGui::Text("Selected Mesh: %s", m.id.c_str());
		ImGui::Text("Triangles: %d", m.numTriangles);
		ImGui::Separator();

		// Name / ID (editable)
		{
			char nameBuf[64] = {0};
			std::strncpy(nameBuf, m.id.c_str(), sizeof(nameBuf) - 1);
			if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
			{
				m.id = std::string(nameBuf);
				if (m_scene) m_scene->m_meshesDirty = true;           // mark dirty so renderer re-uploads
				if (m_resetAccumulation) *m_resetAccumulation = true;
			}
		}

		ImGui::Text("Material");
		const char* materialItems[] = { "Diffuse", "Metal", "Dielectric" , "Light"};
		int matType = (int)m.material.type;
		if (ImGui::Combo("Type", &matType, materialItems, IM_ARRAYSIZE(materialItems)))
		{
			m.material.type = (MaterialType)matType;
			if (m_scene) m_scene->m_meshesDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		float albedo[3] = { m.material.albedo.x, m.material.albedo.y, m.material.albedo.z };
		if (ImGui::ColorEdit3("Albedo", albedo))
		{
			m.material.albedo.x = albedo[0];
			m.material.albedo.y = albedo[1];
			m.material.albedo.z = albedo[2];
			if (m_scene) m_scene->m_meshesDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		float emission[3] = { m.material.emission.x, m.material.emission.y, m.material.emission.z };
		if (ImGui::DragFloat3("Emission", emission))
		{
			m.material.emission.x = emission[0];
			m.material.emission.y = emission[1];
			m.material.emission.z = emission[2];
			if (m_scene) m_scene->m_meshesDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		if (ImGui::SliderFloat("Roughness", &m.material.roughness, 0.0f, 1.0f))
		{
			if (m_scene) m_scene->m_meshesDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}
		if (ImGui::SliderFloat("IOR", &m.material.ior, 1.0f, 3.0f))
		{
			if (m_scene) m_scene->m_meshesDirty = true;
			if (m_resetAccumulation) *m_resetAccumulation = true;
		}

		ImGui::Separator();
	}
	else
	{
		ImGui::Text("No object selected");
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

	ImGui::Separator();
	if (ImGui::DragFloat("Background light", m_backgroundStrength, 0.0005f, 0.001f, 1.0f, "%.3f"))
	{
		if (m_resetAccumulation) *m_resetAccumulation = true;
	}

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

void UI_handler::bindPointers(std::vector<Sphere>* spheres, int* selectedSphereIndex, bool* spheresDirty, bool* resetAccumulation, std::vector<Mesh>* meshes, int* selectedMeshIndex)
{
	m_spheres = spheres;
	m_selectedSphere = selectedSphereIndex;
	m_spheresDirty = spheresDirty;
	m_resetAccumulation = resetAccumulation;

	m_meshes = meshes;
	m_selectedMesh = selectedMeshIndex;
}

void UI_handler::setScene(Scene* scene, bool* resetAccumulation, float* backgroundStrength)
{
	m_scene = scene;
	m_resetAccumulation_external = resetAccumulation;
	m_backgroundStrength = backgroundStrength;
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