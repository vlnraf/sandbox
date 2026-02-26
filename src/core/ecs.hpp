#pragma once

#include <glm/glm.hpp>
#include <stdarg.h>

#include "arena.hpp"
#include "renderer/texture.hpp"
#include "coreapi.hpp"
#include "types.hpp"

#define MAX_ENTITIES 50000
#define MAX_COMPONENTS 50000
#define MAX_COMPONENT_TYPE 500

#define NULL_ENTITY UINT32_MAX

#define ECS_DECLARE_COMPONENT(TYPE) \
    uint32_t TYPE##_component_id; 

#define ECS_DECLARE_COMPONENT_EXTERN(TYPE) \
    CORE_API uint32_t TYPE##_component_id; 

#define registerComponent(TYPE)                                \
    TYPE##_component_id = registerComponentImpl(#TYPE, sizeof(TYPE)); \

#define ECS_TYPE(TYPE) TYPE##_component_id

#define view(...) \
    viewImpl(sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), (uint32_t[]){__VA_ARGS__})

#define getComponent(e, TYPE) \
    ((TYPE*)getComponentImpl(e, TYPE##_component_id))

#define removeComponent(e, TYPE) \
    (removeComponentImpl(e, TYPE##_component_id))

#define hasComponent(e, TYPE) \
    (hasComponentImpl(e, TYPE##_component_id))

#define pushComponent(e, TYPE, value_ptr) \
    pushComponentImpl(e, TYPE##_component_id, (const void*)value_ptr)


typedef uint32_t Entity;
//TODO: preallocate a vector for each type of component and store data in there to have contigous memory
struct Component{
    //Entity entity;
    void* data;
};

struct Components{
    void* elements;
    size_t count;
    size_t elementSize;
};

struct EntityArray{
    //Entity entities[MAX_ENTITIES];
    Entity* entities;
    size_t count = 0;
};

struct SparseSet{
    size_t entityToComponentCount;
    size_t entityToComponentSize;
    uint32_t* entityToComponent;

    Components components;
};

struct DenseToSparse{
    size_t entityCount;
    size_t entitySize;
    uint32_t* entity;
};

struct ComponentRegistry{
    size_t componentsCount;
    size_t componentsSize;
    size_t* components;
};


struct Ecs{
    Entity entities;
    Arena arena;
    Arena frameArena;
    SparseSet* sparse;
    DenseToSparse* denseToSparse;

    char names[MAX_COMPONENT_TYPE][MAX_COMPONENT_TYPE];

    size_t removedEntitiesCount = 0;
    size_t* removedEntities;
    size_t componentId = 1; // we will use 0 as invalid component
    size_t entitiesCount = 0;
};

CORE_API void importBaseModule();
CORE_API Ecs* initEcs(Arena* arena);
CORE_API Entity createEntity();
CORE_API size_t registerComponentImpl(const char* name, const size_t size);
CORE_API void pushComponentImpl(const Entity id, const size_t componentName, const void* data);
CORE_API bool hasComponentImpl(const Entity entity, const size_t componentName);
CORE_API void* getComponentImpl(Entity entity, const size_t componentName);
CORE_API void removeComponentImpl(Entity entity, const size_t componentName);
CORE_API EntityArray viewImpl(uint32_t count, uint32_t* types);
CORE_API void removeEntity(const Entity entity);
CORE_API void destroyEcs();
CORE_API void ecsEndFrame();
CORE_API void clearEcs();

extern ECS_DECLARE_COMPONENT_EXTERN(TransformComponent);
struct TransformComponent{
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
};

extern ECS_DECLARE_COMPONENT_EXTERN(DirectionComponent);
struct DirectionComponent{
    glm::vec2 dir;
};

extern ECS_DECLARE_COMPONENT_EXTERN(VelocityComponent);
struct VelocityComponent{
    glm::vec2 vel;
};

extern ECS_DECLARE_COMPONENT_EXTERN(SpriteComponent);
struct SpriteComponent{
    enum PivotType {PIVOT_CENTER, PIVOT_BOT_LEFT};
    Texture* texture;
    char textureName[512] = {0};
    //std::string textureName;

    // Pixel-based texture coordinates (for atlas sampling)
    Rect sourceRect = {.pos = {0, 0}, .size = {0, 0}};

    // Physical size for rendering (scale)
    glm::vec2 size = {0, 0};

    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

    bool flipX = false;
    bool flipY = false;

    bool ySort = false;
    float ySortOffset = 0.0f;  // Y offset for depth sorting (relative to position)

    float layer = 1.0f;
    bool visible = true;
};

extern ECS_DECLARE_COMPONENT_EXTERN(PersistentTag);
struct PersistentTag{};

extern ECS_DECLARE_COMPONENT_EXTERN(AnimationComponent);
struct AnimationComponent{
    char animationId[512];
    char previousId[512];

    uint16_t currentFrame = 0;
    uint16_t frameCount = 0;
    float elapsedTime = 0;
};

extern ECS_DECLARE_COMPONENT_EXTERN(Parent);
struct Parent{
    Entity entity;
};

extern ECS_DECLARE_COMPONENT_EXTERN(Child);
struct Child{
    Entity entity[10]; //NOTE: right now max 10 childs for each entity
    size_t count = 0;
};
