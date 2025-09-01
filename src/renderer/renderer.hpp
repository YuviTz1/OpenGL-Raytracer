#pragma once
#include "shader_class.hpp"
#include "camera.hpp"

class Renderer
{
public:
	Shader m_QuadShader;
	Shader m_computeShader;
	unsigned int m_VAO = NULL;
	unsigned int m_cameraUBO = NULL;
	unsigned int m_screenTex = NULL;
	camera m_camData;

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

	void processInput(GLFWwindow* window);

private:
	int m_width;
	int m_height;

	void InitScreenTexture();
	void InitCameraUBO();
	void InitComputeShader();
};