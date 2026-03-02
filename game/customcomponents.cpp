#include "customcomponents.hpp"

ECS_DECLARE_COMPONENT(PlayerTag)
ECS_DECLARE_COMPONENT(AsteroidTag)
ECS_DECLARE_COMPONENT(AccelerationComponent)
ECS_DECLARE_COMPONENT(MassComponent)

void importCustomComponentModule(){
    registerComponent(PlayerTag);
    registerComponent(AsteroidTag);
    registerComponent(AccelerationComponent);
    registerComponent(MassComponent);
}