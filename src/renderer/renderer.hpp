#pragma once
#include "shader_class.hpp"
#include "camera.hpp"

struct CameraData {
	glm::vec4 position;
	glm::vec4 front;
	glm::vec4 up;
	glm::vec4 right;
	glm::vec2 fovAndAspect;
	glm::vec2 padding;
};

class Renderer
{
public:
	Shader m_QuadShader;
	Shader m_computeShader;
	unsigned int m_VAO = NULL;
	unsigned int m_cameraUBO = NULL;
	unsigned int m_screenTex = NULL;
	Camera camera;
	float deltaTime = 0.0f;

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

private:
	int m_width;
	int m_height;

	void InitScreenTexture();
	void InitCameraUBO();
	void InitComputeShader();
};