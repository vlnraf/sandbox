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

enum UnitType{
    UNIT_PLAYER,
    UNIT_ENEMY
};

enum UnitStatus{
    STATUS_ALIVE,
    STATUS_DEATH
};

enum ActionType{
    ACTION_NONE,
    ACTION_MOVE,
    ACTION_ATTACK
};

struct Unit{
    UnitType    type;
    UnitStatus  status;
    IVec2       gridPos;
    IVec2       destPos;
    int         actionPoints;
    int         movement;
    int         hp;
    int         dmg;
    int         attackRange;

    //player control
    bool selected;
    ActionType actionType;
    
    //rendering
    //NOTE: shoulde we separate the logic and rendering data??
    String8     dName;      //Debug name
    Vec2        worldPos;
    float       a;
    Color       color; //TODO: change with textures or shaders
};

struct Cell{
    bool    walkable;
    Color   color; //TODO: change with textures or shaders
};


struct WorldGrid{
    IVec2 size;
    Cell* cell;
    float cellSize;
};


struct GameState{
    Arena* arena;

    //UiState* uiState;
    Font        f;
    Texture     gameTextures[MAX_GAME_TEXTURES];
    Vec2        gameSize;

    //world
    WorldGrid   worldGrid;

    //units
    Unit*       units;
    int         maxUnits;

    //turn system
    int         turn; //TODO: swap into a queue of turns?
    Unit**        turnQueue;
    int         turnCount;
    int         turnUnits;

    //rendering
    OrtographicCamera   mainCamera;
    RenderTexture       finalTexture;

};

extern "C" {
    GAME_API const char* applicationSetup();
    GAME_API uint32_t gameStart(Arena* gameArena);
    GAME_API void gameRender(Arena* gameArena, float dt);
    GAME_API void gameUpdate(Arena* gameArena, float dt);
    //GAME_API GameState* gameReload(GameState* gameState, Renderer* renderer, const char* filePath);
    GAME_API void gameStop(Arena* gameArena);
}