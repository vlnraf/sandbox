#pragma once

#include "math.hpp"

#include "core/coreapi.hpp"
#include "renderer/fontmanager.hpp"
#include "renderer/texture.hpp"
#include "core/mystring.hpp"

struct UIState{
    Vec2 mousePos;
    Vec2 screenSize;
    Vec2 canvasSize;
    Font* font;

    uint32_t active;
    uint32_t hot;
    uint32_t id = 1;
};

UIState* initUI(Arena* arena);
void destroyUI();

CORE_API void beginUiFrame(Vec2 canvasPos, Vec2 canvasSize);
CORE_API void endUiFrame();
CORE_API bool pointRectIntersection(Vec2 mousePos, Vec2 pos, Vec2 size);
CORE_API bool UiButton(const char* text, Vec2 pos, Vec2 size, Vec2 rotation); //Deprecated
CORE_API void UiText(const char* text, Vec2 pos, float scale); //Deprecated
CORE_API bool UiButton(String8 text, Vec2 pos, Vec2 size, Vec2 rotation);
CORE_API void UiText(String8 text, Vec2 pos, float scale);
//CORE_API void UiImage(Texture texture, Vec2 pos, Vec2 rotation);
//CORE_API void UiImage(Texture texture, Vec2 pos, Vec2 size, Vec2 rotation, Vec2 index, Vec2 offset);
CORE_API int calculateTextHeight(Font* font, const char* text, float scale);
CORE_API void setFontUI(Font* font);
CORE_API Font* getFontUI();
CORE_API void UIsetScreenSize(float width, float height);