#pragma once

#include <stdint.h>

struct Rect{
    float x, y, w, h;
};

struct Color{
    union{
        float c[4];
        struct { float r, g, b, a; };
    };
};

inline Color ColorFromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a){
    Color c;
    c.r = r/255.0f; c.g = g/255.0f; c.b = b/255.0f; c.a = a/255.0f;
    return c;
}

inline Color ColorFromHex(uint32_t hex){
    return ColorFromRGBA(
        (hex >> 24) & 0xFF,
        (hex >> 16) & 0xFF,
        (hex >>  8) & 0xFF,
        (hex      ) & 0xFF
    );
}

#define COLOR_WHITE    ColorFromRGBA(255,255,255,255)
#define COLOR_BLACK    ColorFromRGBA(0,0,0,255)
#define COLOR_RED      ColorFromRGBA(255,0,0,255)
#define COLOR_GREEN    ColorFromRGBA(0,255,0,255)
#define COLOR_BLUE     ColorFromRGBA(0,0,255,255)
#define COLOR_YELLOW   ColorFromRGBA(255,255,0,255)
#define COLOR_PURPLE   ColorFromRGBA(255,0,255,255)
#define COLOR_SKY_BLUE ColorFromRGBA(0,255,255,255)
