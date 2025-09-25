#include "scene.hpp"
#include <fstream>
#include <cstdint>
#include <filesystem> // Add this for file existence checks

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

bool Scene::SaveToFile(const std::string& path)
{
    // Always create or overwrite the file
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;

	//todo: check if the file is in the project directory (security!?)

    uint32_t version = 1;
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint32_t count = static_cast<uint32_t>(m_spheres.size());
    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const Sphere& s : m_spheres) {
        // id
        uint32_t idlen = static_cast<uint32_t>(s.id.size());
        ofs.write(reinterpret_cast<const char*>(&idlen), sizeof(idlen));
        if (idlen > 0) ofs.write(s.id.data(), idlen);

        // position (vec4)
        float pos[4] = { s.position.x, s.position.y, s.position.z, s.position.w };
        ofs.write(reinterpret_cast<const char*>(pos), sizeof(pos));

        // radius
        ofs.write(reinterpret_cast<const char*>(&s.radius), sizeof(s.radius));

        // material
        uint32_t mtype = static_cast<uint32_t>(s.material.type);
        ofs.write(reinterpret_cast<const char*>(&mtype), sizeof(mtype));

        float alb[4] = { s.material.albedo.r, s.material.albedo.g, s.material.albedo.b, s.material.albedo.a };
        ofs.write(reinterpret_cast<const char*>(alb), sizeof(alb));

        ofs.write(reinterpret_cast<const char*>(&s.material.roughness), sizeof(s.material.roughness));
        ofs.write(reinterpret_cast<const char*>(&s.material.ior), sizeof(s.material.ior));
    }

    return ofs.good();
}

bool Scene::LoadFromFile(const std::string& path, bool& shouldResetAccumulation)
{
    // Check if file exists before loading
    if (!std::filesystem::exists(path)) return false;

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    uint32_t version = 0;
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!ifs) return false;
    if (version != 1) {
        // unsupported version
        return false;
    }

    uint32_t count = 0;
    ifs.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!ifs) return false;
    if (count > static_cast<uint32_t>(MAX_SPHERES)) {
        // prevent runaway allocations / corrupted files
        return false;
    }

    std::vector<Sphere> newSpheres;
    newSpheres.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        Sphere s;

        uint32_t idlen = 0;
        ifs.read(reinterpret_cast<char*>(&idlen), sizeof(idlen));
        if (!ifs) return false;
        if (idlen > 0) {
            std::string id;
            id.resize(idlen);
            ifs.read(&id[0], idlen);
            if (!ifs) return false;
            s.id = std::move(id);
        }
        else {
            s.id.clear();
        }

        float pos[4];
        ifs.read(reinterpret_cast<char*>(pos), sizeof(pos));
        if (!ifs) return false;
        s.position = glm::vec4(pos[0], pos[1], pos[2], pos[3]);

        ifs.read(reinterpret_cast<char*>(&s.radius), sizeof(s.radius));
        if (!ifs) return false;

        uint32_t mtype = 0;
        ifs.read(reinterpret_cast<char*>(&mtype), sizeof(mtype));
        if (!ifs) return false;
        s.material.type = static_cast<MaterialType>(mtype);

        float alb[4];
        ifs.read(reinterpret_cast<char*>(alb), sizeof(alb));
        if (!ifs) return false;
        s.material.albedo = glm::vec4(alb[0], alb[1], alb[2], alb[3]);

        ifs.read(reinterpret_cast<char*>(&s.material.roughness), sizeof(s.material.roughness));
        if (!ifs) return false;
        ifs.read(reinterpret_cast<char*>(&s.material.ior), sizeof(s.material.ior));
        if (!ifs) return false;

        newSpheres.push_back(std::move(s));
    }

    m_spheres = std::move(newSpheres);
    m_spheresDirty = true;
    m_selectedSphereIndex = (m_spheres.empty() ? -1 : static_cast<int>(m_spheres.size() - 1));
    shouldResetAccumulation = true;

    return true;
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