# Entity Component System (ECS) Overview

The Entity Component System (ECS) provides a data-oriented approach to game entity management, separating data from behavior and enabling efficient processing of large numbers of entities.

## What is ECS?

Entity Component System is an architectural pattern that organizes game objects as collections of reusable components rather than hierarchical class structures.

### Traditional OOP vs ECS

```
┌─────────────────────────────────────────────────────────┐
│                    Traditional OOP                      │
│                                                         │
│  ┌────────────────────────────────────────────────────┐ │
│  │                    GameEntity                      │ │
│  │  • Position, Rotation, Scale                       │ │
│  │  • Health, Armor                                   │ │
│  │  • Mesh, Material                                  │ │
│  │  • AI State                                        │ │
│  │  • Physics Body                                    │ │
│  │                                                    │ │
│  │  virtual void update(float dt)                     │ │
│  │  virtual void render(Renderer& r)                  │ │
│  └────────────────────────────────────────────────────┘ │
│                                                         │
│  Problem: Most entities don't use all fields/methods    │
│  Issue: Tight coupling between systems                  │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                         ECS                             │
│                                                         │
│  Entity ID: 42                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │
│  │  Position   │  │   Mesh      │  │   Health    │      │
│  │  x, y, z    │  │  meshId     │  │  current    │      │
│  │  rotation   │  │  materialId │  │  maximum    │      │
│  └─────────────┘  └─────────────┘  └─────────────┘      │
│                                                         │
│  Entity ID: 43                                          │
│  ┌─────────────┐  ┌─────────────┐                       │
│  │  Position   │  │   Mesh      │                       │
│  │  x, y, z    │  │  meshId     │                       │
│  │  rotation   │  │  materialId │                       │
│  └─────────────┘  └─────────────┘                       │
│                                                         │
│  Benefit: Data is stored contiguously for cache         │
│  Advantage: Systems can process entities efficiently    │
└─────────────────────────────────────────────────────────┘
```

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│                     Application                     │
│                                                     │
│  • Runtime management                               │
│  • Input handling                                   │
│  • Frame timing                                     │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                      Registry                       │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │                    Entities                    │ │
│  │  • Entity ID pool                              │ │
│  │  • Lifecycle management                        │ │
│  │  • Component storage                           │ │
│  └────────────────────────────────────────────────┘ │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │                    Systems                     │ │
│  │  • Physics system                              │ │
│  │  • Rendering system                            │ │
│  │  • AI system                                   │ │
│  │  • Audio system                                │ │
│  └────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                   Component Data                    │
│                                                     │
│  • Position components                              │
│  • Rotation components                              │
│  • Mesh components                                  │
│  • Health components                                │
│  • Physics components                               │
│                                                     │
│  Data stored contiguously for cache efficiency      │
└─────────────────────────────────────────────────────┘
```

## Core Concepts

### Entities

Entities are lightweight identifiers that reference game objects:

```cpp
namespace engine::ecs {
    using Entity = std::uint32_t;
    inline constexpr Entity kInvalidEntity = 0;
}

// Entity usage
engine::ecs::Entity entity = registry.createEntity();
bool isValid = (entity != engine::ecs::kInvalidEntity);
```

### Components

Components contain data about entities:

```cpp
// Position component
struct PositionComponent {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

// Mesh component (use procedural geometry - model parsing not yet implemented)
struct MeshComponent {
    std::shared_ptr<engine::Mesh> mesh;
    unsigned int materialId = 0;
    bool castShadow = true;
};

// Health component
struct HealthComponent {
    float currentHealth = 100.0f;
    float maximumHealth = 100.0f;
    bool isImmune = false;
};
```

### Systems

Systems process entities based on their components:

```cpp
// Physics system
class PhysicsSystem {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) {
        // Process all entities with position and physics components
        for (const auto& entity : registry.query<PositionComponent, PhysicsComponent>()) {
            const auto& physics = registry.getComponent<PhysicsComponent>(entity);
            auto& position = registry.getComponentMutable<PositionComponent>(entity);

            // Apply gravity
            position.position.y -= 9.81f * deltaTime;
        }
    }
};

// Rendering system
class RenderingSystem {
public:
    void render(engine::ecs::Registry& registry, engine::Renderer& renderer) {
        for (const auto& entity : registry.query<PositionComponent, MeshComponent>()) {
            const auto& position = registry.getComponent<PositionComponent>(entity);
            const auto& mesh = registry.getComponent<MeshComponent>(entity);

            // Create transform from position component
            engine::Transform transform;
            transform.position = position.position;
            transform.rotation = position.rotation;

            renderer.draw(mesh.mesh, material, transform, frameUniforms);
        }
    }
};
```

## Registry API

### Creating and Managing Entities

```cpp
// Create new entity
engine::ecs::Entity entity = registry.createEntity();

// Destroy entity
registry.destroyEntity(entity);

// Check if entity exists
bool exists = registry.exists(entity);
```

### Component Management

```cpp
// Add component to entity
registry.addComponent<PositionComponent>(entity, {x, y, z});

// Get component (const)
const PositionComponent& pos = registry.getComponent<PositionComponent>(entity);

// Get mutable component
auto& mutablePos = registry.getComponentMutable<PositionComponent>(entity);

// Remove component
registry.removeComponent<PositionComponent>(entity);

// Check if entity has component
bool hasComponent = registry.hasComponent<PositionComponent>(entity);
```

### Entity Queries

```cpp
// Query for entities with specific components
auto entities = registry.query<PositionComponent, MeshComponent>();

// Process queried entities
for (const auto& entity : entities) {
    const auto& pos = registry.getComponent<PositionComponent>(entity);
    const auto& mesh = registry.getComponent<MeshComponent>(entity);

    // Process entity
}
```

## Usage Examples

### Creating a Player Character

```cpp
// Create player entity
engine::ecs::Entity player = registry.createEntity();

// Add position component
registry.addComponent<PositionComponent>(player, {0.0f, 1.0f, 0.0f});

// Add mesh component (use procedural geometry - model parsing not yet implemented)
auto playerMesh = std::make_shared<engine::Mesh>(playerGeometry);
registry.addComponent<MeshComponent>(player, {playerMesh, materialId, true});

// Add health component
registry.addComponent<HealthComponent>(player, {100.0f, 100.0f, false});

// Add AI component (if NPC)
registry.addComponent<AiComponent>(player, {AiState::Idle, difficulty});
```

### Implementing a Rendering System

```cpp
class RenderingSystem {
public:
    void render(engine::ecs::Registry& registry, engine::Renderer& renderer) {
        // Query for all entities that can be rendered
        auto renderableEntities = registry.query<PositionComponent, MeshComponent>();

        // Sort by material for batching
        std::sort(renderableEntities.begin(), renderableEntities.end(),
            [&registry](engine::ecs::Entity a, engine::ecs::Entity b) {
                const auto& meshA = registry.getComponent<MeshComponent>(a);
                const auto& meshB = registry.getComponent<MeshComponent>(b);
                return meshA.materialId < meshB.materialId;
            });

        // Render entities
        for (const auto& entity : renderableEntities) {
            const auto& position = registry.getComponent<PositionComponent>(entity);
            const auto& mesh = registry.getComponent<MeshComponent>(entity);

            engine::Transform transform;
            transform.position = position.position;
            transform.rotation = position.rotation;
            transform.scale = position.scale;

            renderer.draw(mesh.mesh, material, transform, frameUniforms);
        }
    }
};
```

### Physics System Implementation

```cpp
class PhysicsSystem {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) {
        // Query for entities with physics components
        auto physicsEntities = registry.query<PositionComponent, PhysicsComponent>();

        for (const auto& entity : physicsEntities) {
            const auto& physics = registry.getComponent<PhysicsComponent>(entity);
            auto& position = registry.getComponentMutable<PositionComponent>(entity);

            // Apply forces
            Vec3 force = Vec3{0.0f, -9.81f * physics.mass, 0.0f}; // Gravity

            // Update velocity
            Vec3 acceleration = force / physics.mass;
            position.velocity += acceleration * deltaTime;

            // Update position
            position.position += position.velocity * deltaTime;
        }
    }
};
```

## Best Practices

### Component Design

1. **Keep Components Small**: Components should contain minimal, focused data
2. **Use Value Types**: Prefer value types over pointers when possible
3. **Avoid Dependencies**: Components shouldn't reference other components directly
4. **Make Components Serializable**: Allow easy save/load functionality

### System Design

1. **Process Data Efficiently**: Use contiguous memory access patterns
2. **Minimize Queries**: Query once per frame rather than repeatedly
3. **Sort Entities**: Sort by material or type for better cache performance
4. **Parallel Processing**: Design systems to be thread-safe when possible

### Performance Optimization

1. **Component Placement**: Group frequently accessed components together
2. **Entity Pooling**: Reuse entity IDs to reduce allocation overhead
3. **Batch Operations**: Process entities in groups rather than individually
4. **Profile Regularly**: Use profiling tools to identify bottlenecks

## Related Documentation

- [Entities and Registry](entities-and-registry.md) - Detailed entity management
- [Components and Systems](components-and-systems.md) - Component design and system implementation
- [Runtime Integration](../runtime/overview.md) - Using ECS in game runtimes
