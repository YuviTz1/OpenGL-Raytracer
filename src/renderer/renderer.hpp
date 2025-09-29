#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "shader_class.hpp"
#include "camera.hpp"
#include "sphere.hpp"
#include "../engine/scene.hpp"

struct CameraData {
	glm::vec4 position;
	glm::vec4 front;
	glm::vec4 up;
	glm::vec4 right;
	glm::vec2 fovAndAspect;
	glm::vec2 padding;
};

struct AccumulationData {
	unsigned int frameCount;
	unsigned int padding[3];
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

	float backgroundStrength = 0.8f;

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