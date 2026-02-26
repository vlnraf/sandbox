#include "platform/platform.hpp"
#include "core.hpp"
#include <string.h>

// Forward declare game functions that will be statically linked
extern "C" {
    void gameStart(Arena* gameArena);
    void gameRender(Arena* gameArena, float dt);
    void gameUpdate(Arena* gameArena, float dt);
    void gameStop(Arena* gameArena);
}

// Assign function pointers to statically linked game functions
GameStart*  platformGameStart  = gameStart;
GameRender* platformGameRender = gameRender;
GameUpdate* platformGameUpdate = gameUpdate;
GameStop*   platformGameStop   = gameStop;

void platformUnloadGame(){
    // No-op for web (no DLL unloading)
}

void platformLoadGame(const char* dllName){
    // No-op for web (statically linked)
}

bool platformReloadGame(const char* dllName){
    // Hot-reload not supported on web
    return false;
}

void memSet(void* dst, int value, size_t size){
    memset(dst, value, size);
}

void memCopy(void* dst, const void* src, size_t size){
    memcpy(dst, src, size);
}