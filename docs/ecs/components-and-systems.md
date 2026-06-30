# Components and Systems

This document covers component design, system implementation, and best practices for building efficient ECS architectures.

## Component Design Principles

### Component Characteristics

Good components should follow these principles:

```
┌─────────────────────────────────────────────────────────┐
│                 Component Design Rules                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. DATA ONLY                                           │
│     • No behavior or methods                            │
│     • Pure data containers                              │
│     • Value types preferred                             │
│                                                         │
│  2. REUSABLE                                            │
│     • Can be added/removed dynamically                  │
│     • Self-contained data                               │
│     • No dependencies on other components               │
│                                                         │
│  3. SMALL                                               │
│     • Minimal footprint                                 │
│     • Cache-friendly sizes                              │
│     • Avoid large allocations                           │
│                                                         │
│  4. SERIALIZABLE                                        │
│     • Easy to save/load                                 │
│     • Platform-independent format                       │
│     • Version-compatible                                │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Component Examples

```cpp
// Good component design
struct PositionComponent {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct VelocityComponent {
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float maxSpeed = 10.0f;
};

struct HealthComponent {
    float currentHealth = 100.0f;
    float maximumHealth = 100.0f;
    bool isImmune = false;
};

struct MeshComponent {
    std::shared_ptr<engine::Mesh> mesh;  // Use procedural geometry (model parsing not yet implemented)
    unsigned int materialId = 0;
    bool castShadow = true;
};

// Bad component design (to avoid)
struct BadComponent {
    Vec3 position;
    void update(float dt) { /* No methods! */ }
    std::shared_ptr<OtherComponent> other; // No dependencies!
    std::vector<float> largeArray(10000);  // Too large!
};
```

## System Architecture

### System Types and Patterns

```cpp
// Update system - processes entities every frame
class UpdateSystem {
public:
    virtual void update(float deltaTime, engine::ecs::Registry& registry) = 0;
};

// Input system - handles player input
class InputSystem : public UpdateSystem {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) override {
        // Process keyboard input
        auto controllableEntities = registry.query<PositionComponent, InputComponent>();

        for (const auto& entity : controllableEntities) {
            const auto& input = registry.getComponent<InputComponent>(entity);
            auto& position = registry.getComponentMutable<PositionComponent>(entity);

            if (input.forward) {
                position.position.z -= 5.0f * deltaTime;
            }
        }
    }
};

// Physics system - handles physical interactions
class PhysicsSystem : public UpdateSystem {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) override {
        auto physicsEntities = registry.query<PositionComponent, PhysicsComponent>();

        for (const auto& entity : physicsEntities) {
            const auto& physics = registry.getComponent<PhysicsComponent>(entity);
            auto& position = registry.getComponentMutable<PositionComponent>(entity);

            // Apply gravity
            position.velocity.y -= 9.81f * deltaTime;

            // Update position
            position.position += position.velocity * deltaTime;
        }
    }
};

// Rendering system - handles graphics operations
class RenderingSystem {
public:
    void render(engine::ecs::Registry& registry, engine::Renderer& renderer) {
        auto renderableEntities = registry.query<PositionComponent, MeshComponent>();

        for (const auto& entity : renderableEntities) {
            const auto& position = registry.getComponent<PositionComponent>(entity);
            const auto& mesh = registry.getComponent<MeshComponent>(entity);

            engine::Transform transform;
            transform.position = position.position;
            transform.rotation = position.rotation;

            renderer.draw(mesh.mesh, material, transform, frameUniforms);
        }
    }
};
```

### System Execution Order

```cpp
// System manager handles execution order
class SystemManager {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) {
        // Update systems in order
        inputSystem->update(deltaTime, registry);
        physicsSystem->update(deltaTime, registry);
        aiSystem->update(deltaTime, registry);
        animationSystem->update(deltaTime, registry);
    }

    void render(engine::ecs::Registry& registry, engine::Renderer& renderer) {
        renderingSystem->render(registry, renderer);
    }

private:
    std::unique_ptr<InputSystem> inputSystem;
    std::unique_ptr<PhysicsSystem> physicsSystem;
    std::unique_ptr<AiSystem> aiSystem;
    std::unique_ptr<AnimationSystem> animationSystem;
    std::unique_ptr<RenderingSystem> renderingSystem;
};
```

## Component Data Layout

### Entity-Component Storage

The engine uses entity-component storage for optimal cache performance:

```
┌─────────────────────────────────────────────────────┐
│              Component Storage                      │
│                                                     │
│  Position Components (contiguous memory):           │
│  ┌──────────┬──────────┬──────────┬──────────┐      │
│  │   pos1   │   pos2   │   pos3   │   pos4   │      │
│  │ {x,y,z}  │  {x,y,z} │  {x,y,z} │  {x,y,z} │      │
│  └──────────┴──────────┴──────────┴──────────┘      │
│                                                     │
│  Health Components (contiguous memory):             │
│  ┌──────────┬──────────┬──────────┬──────────┐      │
│  │  health1 │  health2 │  health3 │  health4 │      │
│  │ {curr,max}│{curr,max}│{curr,max}│{curr,max}│     │
│  └──────────┴──────────┴──────────┴──────────┘      │
│                                                     │
│  Mesh Components (contiguous memory):               │
│  ┌───────────┬──────────┬──────────┬──────────┐     │
│  │  mesh1    │  mesh2   │  mesh3   │  mesh4   │     │
│  │ {mesh,mat}│{mesh,mat}│{mesh,mat}│{mesh,mat}│     │
│  └───────────┴──────────┴──────────┴──────────┘     │
└─────────────────────────────────────────────────────┘
```

## Advanced System Patterns

### Event-Driven Systems

```cpp
// Event system for inter-system communication
struct Event {
    enum Type {
        EntityCreated,
        EntityDestroyed,
        ComponentAdded,
        ComponentRemoved,
        HealthChanged,
        PositionChanged
    };

    Type type;
    engine::ecs::Entity entity;
};

class EventSystem {
public:
    void postEvent(const Event& event) {
        // Notify all interested systems
        for (const auto& handler : handlers[event.type]) {
            handler(event);
        }
    }

    void subscribe(Event::Type type, std::function<void(const Event&)> handler) {
        handlers[type].push_back(handler);
    }

private:
    std::map<Event::Type, std::vector<std::function<void(const Event&)>>> handlers;
};
```

### System Dependencies

```cpp
// Systems can depend on other systems
class CombatSystem : public UpdateSystem {
public:
    void update(float deltaTime, engine::ecs::Registry& registry) override {
        // Query for entities that can attack
        auto attackers = registry.query<PositionComponent, AttackComponent>();

        for (const auto& attacker : attackers) {
            const auto& position = registry.getComponent<PositionComponent>(attacker);
            const auto& attack = registry.getComponent<AttackComponent>(attacker);

            // Find nearby targets
            auto targets = findNearbyEntities(position, attack.range, registry);

            for (const auto& target : targets) {
                if (registry.hasComponent<HealthComponent>(target)) {
                    applyDamage(target, attack.damage, registry);
                }
            }
        }
    }

private:
    std::vector<engine::ecs::Entity> findNearbyEntities(
        const Vec3& position, float range, engine::ecs::Registry& registry) {

        auto nearby = registry.query<PositionComponent>();
        std::vector<engine::ecs::Entity> result;

        for (const auto& entity : nearby) {
            const auto& targetPos = registry.getComponent<PositionComponent>(entity);
            if (distance(position, targetPos.position) <= range) {
                result.push_back(entity);
            }
        }

        return result;
    }

    void applyDamage(engine::ecs::Entity entity, float damage, engine::ecs::Registry& registry) {
        auto& health = registry.getComponentMutable<HealthComponent>(entity);
        health.currentHealth -= damage;
    }
};
```

## Performance Optimization

### Query Optimization

```cpp
// Good: Single query with multiple component types
auto entities = registry.query<PositionComponent, HealthComponent, MeshComponent>();

// Bad: Multiple queries
auto posEntities = registry.query<PositionComponent>();
for (const auto& entity : posEntities) {
    if (registry.hasComponent<HealthComponent>(entity)) {
        // Additional check needed
    }
}

// Good: Pre-filtered query
auto filteredEntities = registry.query<PositionComponent, HealthComponent>(
    [&registry](engine::ecs::Entity entity) -> bool {
        const auto& health = registry.getComponent<HealthComponent>(entity);
        return health.currentHealth > 0; // Only living entities
    }
);
```

### Memory Optimization

```cpp
// Good: Small, fixed-size components
struct CompactPosition {
    float x, y, z;  // 12 bytes
};

// Bad: Variable-size components
struct ExpansivePosition {
    std::vector<float> coordinates;  // Dynamic allocation
    std::string name;                // Dynamic allocation
};
```

## Best Practices

### Component Design

1. **Keep Components Simple**: Single responsibility per component
2. **Use Value Types**: Avoid pointers and references in components
3. **Make Components Immutable When Possible**: Use const access when reading
4. **Design for Serialization**: Consider save/load requirements early

### System Design

1. **Process Data Efficiently**: Use contiguous memory access patterns
2. **Minimize Queries**: Query once per frame rather than repeatedly
3. **Sort Entities**: Sort by material or type for better cache performance
4. **Parallel Processing**: Design systems to be thread-safe when possible

### Performance Tips

1. **Profile Regularly**: Use profiling tools to identify bottlenecks
2. **Batch Operations**: Process entities in groups
3. **Reuse Memory**: Minimize dynamic allocations
4. **Use Cache-Friendly Layouts**: Organize data for optimal CPU cache usage

## Related Documentation

- [ECS Overview](overview.md) - General ECS concepts
- [Entities and Registry](entities-and-registry.md) - Entity management
- [Runtime Integration](../runtime/overview.md) - Using ECS in game runtimes
