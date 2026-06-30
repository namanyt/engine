# Entities and Registry

This document covers entity management and the registry system that handles entity lifecycle, component storage, and queries.

## Entity System Architecture

```
┌─────────────────────────────────────────────────────┐
│                     Registry                        │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │              Entity Pool                       │ │
│  │  • Free list of available IDs                  │ │
│  │  • Active entity tracking                      │ │
│  │  • ID generation strategy                      │ │
│  └────────────────────────────────────────────────┘ │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │           Component Storage                    │ │
│  │  • Position components: [pos1, pos2, pos3...]  │ │
│  │  • Rotation components: [rot1, rot2, rot3...]  │ │
│  │  • Mesh components: [mesh1, mesh2, mesh3...]   │ │
│  │  • Health components: [health1, health2...]    │ │
│  └────────────────────────────────────────────────┘ │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │              Query System                      │ │
│  │  • Component type filtering                    │ │
│  │  • Entity matching                             │ │
│  │  • Result caching                              │ │
│  └────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

## Entity Lifecycle

### Entity States and Transitions

```
                    ┌─────────────┐
                    │   Created   │
                    │  ID: 1      │
                    └──────┬──────┘
                           │ addComponent()
                           ▼
                    ┌─────────────┐
                    │  Active     │◄──────────────────┐
                    │  Components │                   │
                    │  added      │                   │
                    └──────┬──────┘                   │
                           │ update()                 │
                           ▼                          │
                    ┌─────────────┐                   │
                    │   Running   │───────────────────┤
                    │  Processing │                   │
                    └──────┬──────┘                   │
                           │ destroy()                │
                           ▼                          │
                    ┌─────────────┐                   │
                    │   Dead      │                   │
                    │  Resources  │                   │
                    │  cleaned up │                   │
                    └──────┬──────┘                   │
                           │ return to pool           │
                           ▼                          │
                    ┌─────────────┐                   │
                    │   Freed     │───────────────────┤
                    │ ID recycled │                   │
                    └─────────────┘                   │
                                                      │
                                                      ▼
                                               ┌─────────────┐
                                               │   Created   │
                                               │  ID: 1      │
                                               └─────────────┘
```

## Registry API Reference

### Entity Creation and Destruction

```cpp
// Create a new entity
engine::ecs::Entity createEntity();

// Destroy an existing entity
void destroyEntity(engine::ecs::Entity entity);

// Check if entity exists
bool exists(engine::ecs::Entity entity) const;

// Get total number of active entities
size_t getActiveEntityCount() const;
```

### Component Management

```cpp
// Add a component to an entity
template<typename T>
void addComponent(engine::ecs::Entity entity, const T& component);

// Get a component (const access)
template<typename T>
const T& getComponent(engine::ecs::Entity entity) const;

// Get mutable component (for modification)
template<typename T>
T& getComponentMutable(engine::ecs::Entity entity);

// Remove a component from an entity
template<typename T>
void removeComponent(engine::ecs::Entity entity);

// Check if entity has a specific component
template<typename T>
bool hasComponent(engine::ecs::Entity entity) const;
```

### Entity Queries

```cpp
// Query for entities with specific components
template<typename... T>
std::vector<engine::ecs::Entity> query() const;

// Query with custom filter function
template<typename FilterFunc>
std::vector<engine::ecs::Entity> query(const FilterFunc& filter) const;
```

## Usage Examples

### Basic Entity Management

```cpp
// Initialize registry
engine::ecs::Registry registry;

// Create entities
engine::ecs::Entity player = registry.createEntity();
engine::ecs::Entity enemy = registry.createEntity();
engine::ecs::Entity npc = registry.createEntity();

// Verify entities were created
assert(registry.exists(player));
assert(registry.exists(enemy));
assert(registry.exists(npc));

// Destroy an entity
registry.destroyEntity(npc);
assert(!registry.exists(npc));
```

### Component Operations

```cpp
// Define components
struct PositionComponent {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
};

struct HealthComponent {
    float currentHealth = 100.0f;
    float maximumHealth = 100.0f;
};

// Add components to entity
registry.addComponent<PositionComponent>(player, {10.0f, 5.0f, 0.0f});
registry.addComponent<HealthComponent>(player, {100.0f, 100.0f});

// Access components
const auto& position = registry.getComponent<PositionComponent>(player);
std::cout << "Player position: " << position.position.x << ", "
          << position.position.y << ", " << position.position.z << std::endl;

// Modify components
auto& health = registry.getComponentMutable<HealthComponent>(player);
health.currentHealth -= 10.0f; // Take damage

// Check for component existence
if (registry.hasComponent<HealthComponent>(enemy)) {
    const auto& enemyHealth = registry.getComponent<HealthComponent>(enemy);
    std::cout << "Enemy health: " << enemyHealth.currentHealth << std::endl;
}
```

### Entity Queries

```cpp
// Query for entities with specific components
auto positionEntities = registry.query<PositionComponent>();
std::cout << "Found " << positionEntities.size() << " entities with positions" << std::endl;

// Query for entities with multiple component types
auto movableEntities = registry.query<PositionComponent, HealthComponent>();
for (const auto& entity : movableEntities) {
    const auto& pos = registry.getComponent<PositionComponent>(entity);
    const auto& health = registry.getComponent<HealthComponent>(entity);

    std::cout << "Entity " << entity << " at (" << pos.position.x << ", "
              << pos.position.y << ") with " << health.currentHealth << " HP" << std::endl;
}

// Query with custom filter
auto lowHealthEntities = registry.query<PositionComponent, HealthComponent>(
    [&registry](engine::ecs::Entity entity) -> bool {
        const auto& health = registry.getComponent<HealthComponent>(entity);
        return health.currentHealth < 30.0f; // Less than 30% health
    }
);
```

## Advanced Usage

### Entity Factory Pattern

```cpp
class EntityFactory {
public:
    static engine::ecs::Entity createPlayer(
        engine::ecs::Registry& registry,
        const Vec3& position,
        std::shared_ptr<engine::AssetManager> assetManager) {

        // Create entity
        engine::ecs::Entity entity = registry.createEntity();

        // Add position component
        registry.addComponent<PositionComponent>(entity, {position});

        // Add mesh component (use procedural geometry - model parsing not yet implemented)
        auto playerMesh = std::make_shared<engine::Mesh>(playerGeometry);
        registry.addComponent<MeshComponent>(entity, {playerMesh, materialId, true});

        // Add health component
        registry.addComponent<HealthComponent>(entity, {100.0f, 100.0f});

        // Add input component
        registry.addComponent<InputComponent>(entity, {true, true, false});

        return entity;
    }

    static engine::ecs::Entity createEnemy(
        engine::ecs::Registry& registry,
        const Vec3& position,
        Difficulty difficulty) {
```
