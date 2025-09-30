#include "scene.hpp"
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <algorithm> // added

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

        // write emission
        float emi[4] = { s.material.emission.r, s.material.emission.g, s.material.emission.b, s.material.emission.a };
        ofs.write(reinterpret_cast<const char*>(emi), sizeof(emi));

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

        float emi[4];
        ifs.read(reinterpret_cast<char*>(emi), sizeof(emi));
        if (!ifs) return false;
        s.material.emission = glm::vec4(emi[0], emi[1], emi[2], emi[3]);

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

bool Scene::LoadOBJ(const std::string& filename, Mesh& mesh, bool& shouldResetAccumulation)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    // Optional: we parse vt but do not use it
    std::vector<glm::vec2> texcoords;

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    auto fixIndex = [](int idx, size_t size) -> int {
        // OBJ: positive indices are 1-based; negative indices are relative to the end
        if (idx > 0) return idx - 1;
        if (idx < 0) return static_cast<int>(size) + idx;
        return -1; // 0 is invalid in OBJ
    };

    struct FaceElem { int v = -1; int vt = -1; int vn = -1; };

    auto parseFaceToken = [](const std::string& tok) -> FaceElem {
        // Accepts: v | v/vt | v//vn | v/vt/vn
        FaceElem fe{};
        int fields[3] = { 0, 0, 0 };
        int fieldIdx = 0;

        // Split by '/'
        size_t start = 0;
        while (start <= tok.size() && fieldIdx < 3) {
            size_t pos = tok.find('/', start);
            std::string part = (pos == std::string::npos) ? tok.substr(start) : tok.substr(start, pos - start);
            if (!part.empty()) {
                try {
                    fields[fieldIdx] = std::stoi(part);
                } catch (...) {
                    fields[fieldIdx] = 0;
                }
            } else {
                fields[fieldIdx] = 0; // empty means missing
            }
            ++fieldIdx;
            if (pos == std::string::npos) break;
            start = pos + 1;
        }

        fe.v  = fields[0] == 0 ? -1 : fields[0];
        // If only two fields given, it's v/vt; if three, it's v/vt/vn
        if (fieldIdx >= 2) fe.vt = fields[1] == 0 ? -1 : fields[1];
        if (fieldIdx >= 3) fe.vn = fields[2] == 0 ? -1 : fields[2];
        return fe;
    };

    const int start = static_cast<int>(m_triangles.size());
    int added = 0;

    std::string line;
    while (std::getline(file, line)) {
        // Strip comments
        if (auto hash = line.find('#'); hash != std::string::npos) line.resize(hash);

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        if (prefix == "v") {
            glm::vec3 pos{};
            iss >> pos.x >> pos.y >> pos.z;
            if (!iss.fail()) positions.push_back(pos);
        } else if (prefix == "vn") {
            glm::vec3 norm{};
            iss >> norm.x >> norm.y >> norm.z;
            if (!iss.fail()) normals.push_back(norm);
        } else if (prefix == "vt") {
            glm::vec2 uv{};
            iss >> uv.x >> uv.y;
            if (!iss.fail()) texcoords.push_back(uv);
        } else if (prefix == "f") {
            // Collect face tokens
            std::vector<FaceElem> elems;
            std::string tok;
            while (iss >> tok) {
                if (tok.empty()) continue;
                elems.push_back(parseFaceToken(tok));
            }

            if (elems.size() < 3) continue; // not a face

            // Triangulate fan: (0, i, i+1)
            for (size_t i = 1; i + 1 < elems.size(); ++i) {
                const FaceElem fe[3] = { elems[0], elems[i], elems[i + 1] };
                int vi[3] = { -1, -1, -1 };
                int ni[3] = { -1, -1, -1 };

                bool indicesValid = true;

                for (int k = 0; k < 3; ++k) {
                    // Fix vertex indices
                    vi[k] = fixIndex(fe[k].v, positions.size());
                    if (vi[k] < 0 || static_cast<size_t>(vi[k]) >= positions.size()) {
                        indicesValid = false;
                        break;
                    }
                    // Fix normal indices (optional)
                    if (fe[k].vn != -1) {
                        ni[k] = fixIndex(fe[k].vn, normals.size());
                        if (ni[k] < 0 || static_cast<size_t>(ni[k]) >= normals.size()) {
                            indicesValid = false;
                            break;
                        }
                    }
                }
                if (!indicesValid) {
                    // Skip malformed triangle instead of crashing
                    continue;
                }

                Triangle tri{};
                tri.v0 = positions[vi[0]];
                tri.v1 = positions[vi[1]];
                tri.v2 = positions[vi[2]];

                if (ni[0] >= 0 && ni[1] >= 0 && ni[2] >= 0) {
                    tri.n0 = normals[ni[0]];
                    tri.n1 = normals[ni[1]];
                    tri.n2 = normals[ni[2]];
                } else {
                    // Compute flat normal if not present
                    glm::vec3 e1 = tri.v1 - tri.v0;
                    glm::vec3 e2 = tri.v2 - tri.v0;
                    glm::vec3 n = glm::normalize(glm::cross(e1, e2));
                    if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
                        // Degenerate triangle; skip
                        continue;
                    }
                    tri.n0 = tri.n1 = tri.n2 = n;
                }

                m_triangles.push_back(tri);
                added++;
            }
        }
        // ignore other prefixes
    }

    if (added == 0) return false;

    mesh.startIndex = start;
    mesh.numTriangles = added;
    mesh.endIndex = start + added; // optional
    m_meshes.push_back(mesh);

    m_meshesDirty = true;
    shouldResetAccumulation = true;
    return true;
}

bool Scene::RemoveMesh(int index, bool& shouldResetAccumulation)
{
    shouldResetAccumulation = false;
    if (index < 0 || index >= (int)m_meshes.size()) {
        return false;
    }

    // Determine triangle range to remove, clamped for safety
    const Mesh& victim = m_meshes[index];
    int start = std::clamp(victim.startIndex, 0, (int)m_triangles.size());
    int end   = std::clamp(victim.endIndex,   start, (int)m_triangles.size());
    int count = std::max(0, end - start);

    // Remove triangles for this mesh
    if (count > 0) {
        m_triangles.erase(m_triangles.begin() + start, m_triangles.begin() + end);
    }

    // Adjust subsequent meshes' triangle indices
    for (int i = index + 1; i < (int)m_meshes.size(); ++i) {
        m_meshes[i].startIndex -= count;
        m_meshes[i].endIndex   -= count;
    }

    // Remove the mesh entry
    m_meshes.erase(m_meshes.begin() + index);

    // Update selection
    if (m_meshes.empty()) {
        m_selectedMeshIndex = -1;
    } else {
        if (index >= (int)m_meshes.size()) m_selectedMeshIndex = (int)m_meshes.size() - 1;
        else m_selectedMeshIndex = index;
    }

    m_meshesDirty = true;
    shouldResetAccumulation = true;
    return true;
}

Scene::Scene()
{
    Sphere ground;
    ground.id = "Ground";
    ground.position = glm::vec4(0.0f, -101.0f, -6.0f, 0.0f);
    ground.radius = 100.0f;
    ground.material.type = DIFFUSE;
    ground.material.albedo = glm::vec4(0.3f, 0.3f, 0.7f, 0.0f);
    AddSphere(ground);

    Sphere Sun;
    Sun.id = "Sun";
    Sun.position = glm::vec4(0.0f, 1000.0f, 300.0f, 0.0f);
    Sun.radius = 80.0f;
    Sun.material.type = EMISSIVE;
    Sun.material.albedo = glm::vec4(0.3f, 0.3f, 0.7f, 0.0f);
    Sun.material.emission = glm::vec4(232.0f, 232.0f, 232.0f, 0.0f); // bright "sun" light
    AddSphere(Sun);
}

Scene::~Scene()
{
}