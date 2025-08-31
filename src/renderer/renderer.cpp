#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

#include "stb_image.h"
#include "renderer.hpp"
#include "camera.hpp"

Renderer::Renderer(int width, int height)
	: m_width(width), m_height(height), m_QuadShader("res/vertex.shader", "res/fragment.shader"), m_computeShader("res/compute.shader"), m_camData(90.0f)
{
	InitCameraUBO();
    InitScreenTexture();
    InitComputeShader();
}

Renderer::~Renderer()
{

}

void Renderer::InitScreenTexture()
{
    float vertices[] =
    {
        -1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
        -1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
         1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
    };

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);  
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(0); 

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)12);
    glEnableVertexAttribArray(1);
	m_VAO = VAO;

    unsigned int screenTex;
    glGenTextures(1, &screenTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(screenTex, 1, GL_RGBA32F, m_width, m_height);
    glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	m_screenTex = screenTex;
}

void Renderer::InitCameraUBO()
{
    unsigned int cameraUBO;
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(m_camData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
	m_cameraUBO = cameraUBO;
}

void Renderer::InitComputeShader()
{
    // Get the uniform block index and bind it explicitly
    unsigned int blockIndex = glGetUniformBlockIndex(m_computeShader.ID, "cameraBlock");
    if (blockIndex == GL_INVALID_INDEX) {
        std::cout << "Failed to find CameraBlock uniform block" << std::endl;
    }
    else {
        std::cout << "Found CameraBlock at index: " << blockIndex << std::endl;
        glUniformBlockBinding(m_computeShader.ID, blockIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cameraUBO);
    }
}

void Renderer::processInput(GLFWwindow* window)
{

    glm::vec3 forward = glm::normalize(m_camData.lookat - m_camData.lookfrom);
    glm::vec3 right = glm::normalize(glm::cross(forward, m_camData.up));
    float moveSpeed = 0.05f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_camData.lookfrom += forward * moveSpeed, m_camData.lookat += forward * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_camData.lookfrom -= forward * moveSpeed, m_camData.lookat -= forward * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_camData.lookfrom -= right * moveSpeed, m_camData.lookat -= right * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_camData.lookfrom += right * moveSpeed, m_camData.lookat += right * moveSpeed;

    m_camData.updateVectors();

    // Synchronize radius for consistent movement
    camera_radius = glm::length(m_camData.lookfrom - m_camData.lookat);
}

void Renderer::updateCameraFromAngles()
{
    // Spherical coordinates to cartesian
    float x = camera_radius * cos(glm::radians(camera_pitch)) * cos(glm::radians(camera_yaw));
    float y = camera_radius * sin(glm::radians(camera_pitch));
    float z = camera_radius * cos(glm::radians(camera_pitch)) * sin(glm::radians(camera_yaw));
    m_camData.lookfrom = m_camData.lookat + glm::vec3(x, y, z);
    m_camData.updateVectors();
}

void Renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer) return;

    if (renderer->camera_firstMouse)
    {
        renderer->camera_lastX = float(xpos);
        renderer->camera_lastY = float(ypos);
        renderer->camera_firstMouse = false;
    }

    float xoffset = float(xpos) - renderer->camera_lastX;
    float yoffset = renderer->camera_lastY - float(ypos);
    renderer->camera_lastX = float(xpos);
    renderer->camera_lastY = float(ypos);

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    renderer->camera_yaw -= xoffset;
    renderer->camera_pitch += yoffset;

    // Constrain pitch
    if (renderer->camera_pitch > 89.0f) renderer->camera_pitch = 89.0f;
    if (renderer->camera_pitch < -89.0f) renderer->camera_pitch = -89.0f;

    renderer->updateCameraFromAngles();
}