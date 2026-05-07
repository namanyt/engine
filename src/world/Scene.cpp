#include "world/Scene.h"

#include "graphics/Mesh.h"

namespace engine
{
Scene::Scene() = default;

Scene::~Scene() = default;

Scene::Scene(Scene&&) noexcept = default;

Scene& Scene::operator=(Scene&&) noexcept = default;

Mesh& Scene::ownMesh(std::unique_ptr<Mesh> mesh)
{
    m_ownedMeshes.push_back(std::move(mesh));
    return *m_ownedMeshes.back();
}

void Scene::clearRuntimeViews()
{
    m_objects.clear();
    localLights.clear();
    rayTracingScene.bounds.clear();
}

void Scene::resetWorld()
{
    m_registry.clear();
    clearRuntimeViews();
    m_ownedMeshes.clear();
}
} // namespace engine
