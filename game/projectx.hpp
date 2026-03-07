#pragma once

#if defined(_WIN32)
    #ifdef GAME_EXPORT
        #define GAME_API __declspec(dllexport)
    #else
        #define GAME_API __declspec(dllimport)
    #endif
#else
    // Linux / macOS
    #ifdef GAME_EXPORT
        #define GAME_API __attribute__((visibility("default")))
    #else
        #define GAME_API
    #endif
#endif

#include "core.hpp"


#define MAX_GAME_TEXTURES 10
struct UiState;

enum GameTextures{
    PLAYER_TEXTURE = 0,
};

struct Cell{
    bool walkable;
};

enum UnitType{
    UNIT_PLAYER,
    UNIT_ENEMY
};

struct Unit{
    UnitType type;
    glm::ivec2 pos;
};

struct WorldGrid{
    glm::ivec2 size;
    Cell* cell;
    float cellSize;
};


struct GameState{
    Arena* arena;
    OrtographicCamera mainCamera;
    bool restart;

    Font f;
    //UiState* uiState;
    RenderTexture finalTexture;
    Texture gameTextures[MAX_GAME_TEXTURES];
    glm::vec2 gameSize;

    WorldGrid worldGrid;
    Unit* units;

    bool pause = false;
};

extern "C" {
    GAME_API void gameStart(Arena* gameArena);
    GAME_API void gameRender(Arena* gameArena, float dt);
    GAME_API void gameUpdate(Arena* gameArena, float dt);
    //GAME_API GameState* gameReload(GameState* gameState, Renderer* renderer, const char* filePath);
    GAME_API void gameStop(Arena* gameArena);
}