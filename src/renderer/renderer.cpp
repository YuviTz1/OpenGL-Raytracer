#include "renderer.hpp"
#include "stb_image.h"
#include "imgui.h"

struct GPUMaterial {
    int type;              
    int _padA[3];          
    glm::vec4 albedo;      
	glm::vec4 emission;    
    float roughness;       
    float ior;             
	float _padB[2];        
};
static_assert(sizeof(GPUMaterial) == 64, "GPUMaterial mismatch");

struct GPUSphere {
    glm::vec4 position;    
    float radius;          
    int _padC[3];          
    GPUMaterial material;  
};
static_assert(sizeof(GPUSphere) == 96, "GPUSphere mismatch");

struct GPUTriangle {
    glm::vec4 v0, v1, v2; // Triangle vertices
    glm::vec4 n0, n1, n2; // Triangle normals
};

struct GPUMesh {
    GPUMaterial material;   // Material for this mesh
	int triangleCount;   // Number of triangles in the mesh
	int startIndex;     // Start index in the global triangle buffer
	int endIndex;       // End index in the global triangle buffer
	int _padD;       // Padding for alignment
};
static_assert(sizeof(GPUMesh) == 80, "GPUMesh mismatch");

Renderer::Renderer(int width, int height)
	: m_width(width), m_height(height), m_QuadShader("res/vertex.shader", "res/fragment.shader"), m_computeShader("res/compute.shader"), accumulationData{ 0 }, m_frameCount(0)
{
	InitCameraUBO();
    InitScreenTexture();
    InitComputeShader();
	InitSphereSSBO();
	InitMeshSSBO();
	InitAccumulationUBOandTexture();
}

void Renderer::UploadSpheres(Scene& scene)
{
    if (!scene.m_spheresDirty) return;

    std::vector<GPUSphere> gpuData;
    gpuData.reserve(scene.m_spheres.size());

    for (const auto& s : scene.m_spheres)
    {
        GPUSphere gs;
        gs.position = s.position;
        gs.radius = s.radius;
        gs.material.type = static_cast<int>(s.material.type);
        gs.material.albedo = s.material.albedo;
        gs.material.emission = s.material.emission; // copy emission to GPU struct
        gs.material.roughness = s.material.roughness;
        gs.material.ior = s.material.ior;
        gpuData.push_back(gs);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuData.size() * sizeof(GPUSphere), gpuData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    scene.m_spheresDirty = false;
    shouldResetAccumulation = true;
}

void Renderer::UploadMeshes(Scene& scene)
{
    if (!scene.m_meshesDirty) return;

    // 1) Pack triangles
    std::vector<GPUTriangle> gpuTris;
    gpuTris.reserve(scene.m_triangles.size());
    for (const Triangle& t : scene.m_triangles)
    {
        GPUTriangle gt;
        gt.v0 = glm::vec4(t.v0, 0.0f);
        gt.v1 = glm::vec4(t.v1, 0.0f);
        gt.v2 = glm::vec4(t.v2, 0.0f);
        gt.n0 = glm::vec4(t.n0, 0.0f);
        gt.n1 = glm::vec4(t.n1, 0.0f);
        gt.n2 = glm::vec4(t.n2, 0.0f);
        gpuTris.push_back(gt);
    }

    // 2) Pack mesh metadata
    std::vector<GPUMesh> metas;
    metas.reserve(scene.m_meshes.size());
    for (const Mesh& m : scene.m_meshes)
    {
        GPUMesh meta{};
        meta.material.type = static_cast<int>(m.material.type);
        meta.material.albedo = m.material.albedo;
        meta.material.emission = m.material.emission;
        meta.material.roughness = m.material.roughness;
        meta.material.ior = m.material.ior;

        meta.startIndex = m.startIndex;
        meta.triangleCount = m.numTriangles;
		meta.endIndex = m.endIndex;
        metas.push_back(meta);
    }

    // Upload triangles
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triangleSSBO);
    if (!gpuTris.empty())
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuTris.size() * sizeof(GPUTriangle), gpuTris.data(), GL_DYNAMIC_DRAW);
    else
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_triangleSSBO);

    // Upload mesh metas
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshSSBO);
    if (!metas.empty())
        glBufferData(GL_SHADER_STORAGE_BUFFER, metas.size() * sizeof(GPUMesh), metas.data(), GL_DYNAMIC_DRAW);
    else
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_meshSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    scene.m_meshesDirty = false;
    shouldResetAccumulation = true;
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

void Renderer::InitAccumulationUBOandTexture()
{
    // Create accumulation UBO
    glGenBuffers(1, &m_accumulationUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_accumulationUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(AccumulationData), &accumulationData, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_accumulationUBO);

    // Create accumulation texture
    glGenTextures(1, &m_accumulationTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_accumulationTexture);

    glTextureParameteri(m_accumulationTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_accumulationTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_accumulationTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_accumulationTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(m_accumulationTexture, 1, GL_RGBA32F, m_width, m_height);
    glBindImageTexture(1, m_accumulationTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void Renderer::updateAccumulation()
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_accumulationUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccumulationData), &accumulationData);
}

void Renderer::InitCameraUBO()
{
    unsigned int cameraUBO;
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
	m_cameraUBO = cameraUBO;
}

void Renderer::InitComputeShader()
{
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

void Renderer::InitSphereSSBO()
{
    glGenBuffers(1, &m_sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    // Allocate max capacity upfront
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_SPHERES * sizeof(GPUSphere), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::InitMeshSSBO()
{
    // Triangles buffer (binding 2)
    glGenBuffers(1, &m_triangleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_TRIANGLES * sizeof(GPUTriangle), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_triangleSSBO);

    // Mesh metadata buffer (binding 3)
    glGenBuffers(1, &m_meshSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_MESHES * sizeof(GPUMesh), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_meshSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::resetAccumulation() 
{
    accumulationData.frameCount = 0;
    glBindBuffer(GL_UNIFORM_BUFFER, m_accumulationUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccumulationData), &accumulationData);
}

void Renderer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    // Skip if ImGui wants the mouse
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer)
    {
		std::cout << "Renderer pointer is null in mouse_callback" << std::endl;
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (renderer->camera_firstMouse)
    {
        renderer->camera_lastX = xpos;
        renderer->camera_lastY = ypos;
        renderer->camera_firstMouse = false;
    }

    float xoffset = xpos - renderer->camera_lastX;
    float yoffset = renderer->camera_lastY - ypos;

    renderer->camera_lastX = xpos;
    renderer->camera_lastY = ypos;

    renderer->camera.ProcessMouseMovement(xoffset, yoffset);
	renderer->shouldResetAccumulation = true;
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Skip if ImGui wants the mouse wheel
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    //camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void Renderer::processInput(GLFWwindow* window)
{
    // Skip if ImGui wants the keyboard
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
		shouldResetAccumulation = true;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        shouldResetAccumulation = true;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
        shouldResetAccumulation = true;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
        shouldResetAccumulation = true;
    }
}

Renderer::~Renderer()
{

}