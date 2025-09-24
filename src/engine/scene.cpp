#include "scene.hpp"

int Scene::AddSphere(const Sphere& s)
{
    if ((int)m_spheres.size() >= MAX_SPHERES) {
        std::cout << "Max spheres reached\n";
        return -1;
    }
    m_spheres.push_back(s);
    m_spheresDirty = true;
    m_selectedSphereIndex = (int)m_spheres.size() - 1;
    return m_selectedSphereIndex;
}

Scene::Scene()
{
    Sphere ground;
    ground.position = glm::vec4(0.0f, -101.0f, -6.0f, 0.0f);
    ground.radius = 100.0f;
    ground.material.type = DIFFUSE;
    ground.material.albedo = glm::vec4(0.3f, 0.3f, 0.7f, 0.0f);
    AddSphere(ground);
}

Scene::~Scene()
{
}