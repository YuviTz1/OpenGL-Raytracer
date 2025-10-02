#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "shader_class.hpp"
#include "camera.hpp"
#include "sphere.hpp"
#include "../engine/scene.hpp"
#include "../engine/bvh.hpp"

struct CameraData {
	glm::vec4 position;
	glm::vec4 front;
	glm::vec4 up;
	glm::vec4 right;
	glm::vec2 fovAndAspect;
	float halfTanFov;
	int padding;
};

struct AccumulationData {
	unsigned int frameCount;
	unsigned int padding[3];
};

// Statistics gathered on GPU (must match compute shader StatsBuffer layout)
struct RenderStats {
	unsigned int raysSent;
	unsigned int sphereTests;
	unsigned int triangleTests;
	unsigned int sphereHits;    // match compute shader order
	unsigned int triangleHits;  // match compute shader order
	unsigned int bounces;
	unsigned int lightHits;
	unsigned int misses;
};

class Renderer
{
public:
	Shader m_QuadShader;
	Shader m_computeShader;
	unsigned int m_VAO = NULL;
	unsigned int m_cameraUBO = NULL;
	unsigned int m_screenTex = NULL;
	unsigned int m_accumulationUBO = NULL;
	Camera camera;
	float deltaTime = 0.0f;
	int m_frameCount;
	unsigned int m_accumulationTexture;
	AccumulationData accumulationData;

	static constexpr int MAX_SPHERES = 256;
	unsigned int m_sphereSSBO = 0;
	bool shouldResetAccumulation = false;

	// Mesh GPU resources
	static constexpr int MAX_MESHES = 256;
	static constexpr int MAX_TRIANGLES = 1024;
	unsigned int m_meshSSBO = 0;      // MeshMeta buffer (binding 3)
	unsigned int m_triangleSSBO = 0;  // Triangles buffer (binding 2)

	unsigned int m_bvhSSBO = 0;       // binding 5
	unsigned int m_triIndexSSBO = 0;      // binding 6
	static constexpr int MAX_BVH_NODES = 2 * MAX_TRIANGLES - 1;
	int m_bvhNodeCount = 0;

	float backgroundStrength = 0.8f;

	// Stats and debug
	bool debugStatsEnabled = false;
	unsigned int m_statsSSBO = 0; // binding 4
	RenderStats stats{};          // CPU mirror of GPU stats

	unsigned int m_indices[6] =
	{  // note that we start from 0!
		0, 2, 1,
		0, 3, 2
	};

	int camera_radius = 6.0f; // Distance from lookat
	float camera_yaw = -90.0f, camera_pitch = 0.0f;
	bool camera_firstMouse = true;
	float camera_lastX = m_width / 2.0f, camera_lastY = m_height / 2.0f;

	Renderer(int width, int height);
	~Renderer();

	void static mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void static scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void processInput(GLFWwindow* window);
	void updateAccumulation();

	void UploadSpheres(Scene& scene);
	void resetAccumulation();

	// Mesh functions
	void UploadMeshes(Scene& scene);

	// Stats functions
	void InitStatsSSBO();
	void ResetStatsBuffer();
	void ReadStatsBuffer();

private:
	int m_width;
	int m_height;

	void InitScreenTexture();
	void InitCameraUBO();
	void InitComputeShader();
	void InitSphereSSBO();
	void InitAccumulationUBOandTexture();

	// Mesh init
	void InitMeshSSBO();
};