#ifndef COMPONENTS_CUSTOM_HPP
#define COMPONENTS_CUSTOM_HPP

#include "core/ecs.hpp"

extern ECS_DECLARE_COMPONENT(PlayerTag)
struct PlayerTag{}; //Player tag

extern ECS_DECLARE_COMPONENT(AsteroidTag)
struct AsteroidTag{}; //Player tag

extern ECS_DECLARE_COMPONENT(AccelerationComponent)
struct AccelerationComponent{
    glm::vec2 a;
};

void importCustomComponentModule();
#endif