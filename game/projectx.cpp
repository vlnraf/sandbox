#include "projectx.hpp"

void systemRenderSprites(Ecs* ecs, GameState* gameState){
    EntityArray entities = view(ECS_TYPE(TransformComponent), ECS_TYPE(SpriteComponent));

    for(size_t i = 0; i < entities.count; i++){
        Entity entity = entities.entities[i];
        TransformComponent* t= (TransformComponent*) getComponent(entity, TransformComponent);
        SpriteComponent* s= (SpriteComponent*) getComponent(entity, SpriteComponent);
        if(s->visible){
            OrtographicCamera cam = gameState->mainCamera;
            // TODO: move this check in the renderer to cull everything that is not on screen
            // when we are in the world position, not in screen position
            if( (t->position.x <= (cam.position.x - (cam.width / 2))  || t->position.x >= (cam.position.x + (cam.width  / 2))) &&
                (t->position.y <= (cam.position.y - (cam.height / 2)) || t->position.y >= (cam.position.y + (cam.height / 2)))) continue; 
            // Calculate final size from sprite size * transform scale
            glm::vec2 size = s->size * glm::vec2(t->scale.x, t->scale.y);

            // Calculate position for rendering (center of sprite)
            glm::vec3 position = t->position;
            position.z = s->layer;

            // Use sourceRect if set, otherwise default to full texture
            Rect sourceRect = s->sourceRect;
            if(s->sourceRect.size.x == 0 || s->sourceRect.size.y == 0) {
                sourceRect = {.pos = {0, 0}, .size = {(float)s->texture->width, (float)s->texture->height}};
            }

            // Handle UV flipping for flipX/flipY
            if(s->flipX) {
                sourceRect.pos.x += sourceRect.size.x;
                sourceRect.size.x = -sourceRect.size.x;
            }
            if(s->flipY) {
                sourceRect.pos.y += sourceRect.size.y;
                sourceRect.size.y = -sourceRect.size.y;
            }

            // Call renderDrawQuadPro directly
            renderDrawQuadPro(
                position,
                size,
                t->rotation,
                sourceRect,
                {0.5f,0.5f},
                s->texture,
                s->color,
                s->ySort,
                s->ySortOffset  // Pass y-sort offset for depth sorting
            );
        }
    }
}

//void moveSystem(Ecs* ecs, float dt){
//    EntityArray entities = view(ecs, ECS_TYPE(TransformComponent), ECS_TYPE(VelocityComponent), ECS_TYPE(DirectionComponent));
//    for(size_t i = 0; i < entities.count; i++){
//        Entity e = entities.entities[i];
//        TransformComponent* transform = (TransformComponent*) getComponent(ecs, e, TransformComponent);
//        VelocityComponent* velocity  = (VelocityComponent*)  getComponent(ecs, e, VelocityComponent);
//        DirectionComponent* direction  = (DirectionComponent*)  getComponent(ecs, e, DirectionComponent);
//        transform->position += glm::vec3(direction->dir.x * velocity->vel.x * dt, direction->dir.y * velocity->vel.y * dt, 0.0f);
//    }
//}

GAME_API void gameStart(Arena* gameArena, EngineState* engineState){
    if(gameArena->index > 0){
        return;
    }
    GameState* gameState = arenaAllocStruct(gameArena, GameState);
    gameState->arena = gameArena;
    // Resolution-independent camera: shows 640 world units horizontally, 320 vertically, centered at origin
    // Bounds: -320 to +320 horizontally, -160 to +160 vertically
    gameState->restart = false;
    gameState->mainCamera = createCamera(-640.0f / 2, 640.0f / 2, -320.0f / 2, 320.0f / 2);
    setActiveCamera(&gameState->mainCamera);
}

GAME_API void gameRender(Arena* gameArena, EngineState* engine, float dt){}

GAME_API void gameUpdate(Arena* gameArena, EngineState* engineState, float dt){
    clearColor(0.2f, 0.3f, 0.3f, 1.0f);
    beginScene();
    renderDrawFilledRect({10,10}, {50,50}, 0, {1,0,0,1}, 1);
    renderDrawCirclePro({100,100}, 25, {0.5,0.5}, {1,1,1,1}, 1);
    endScene();
}

GAME_API void gameStop(Arena* gameArena, EngineState* engine){
}