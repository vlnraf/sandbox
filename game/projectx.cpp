#include "projectx.hpp"

#define ASSERT(condition, msg) \
    do { \
        if(!(condition)) { \
            LOGERROR("ASSERT FAILED: %s | %s:%d", msg, __FILE__, __LINE__); \
            __builtin_debugtrap(); \
        } \
    } while(0)


#define UI_LAYER 30
#define MAX_RENDER_COMMANDS 1000

enum WidgetFlags{
    WIDGET_NONE = 0,
    WIDGET_FULL_WIDTH   = 1 << 0,  // take full available context width
    WIDGET_FULL_HEIGHT  = 1 << 1,  // take full available context height
    WIDGET_NO_ADVANCE   = 1 << 2,  // don't move cursor after widget (overlay)
    WIDGET_DISABLED     = 1 << 3,  // grayed out, not interactive
    WIDGET_NO_BACKGROUND= 1 << 4,  // skip background rect
    WIDGET_CENTER_TEXT  = 1 << 5,  // center label within widget bounds
    WIDGET_WRAP_TEXT    = 1 << 6,  // wrap text (you had this)
};

enum CommandType{
    UI_RECTANGLE,
    UI_TEXT
};

struct UiRenderCommand{
    CommandType type;
    String8 label;
    float scale;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec4 color;
};

struct UiRenderCommandArray{
    uint32_t lenght;
    UiRenderCommand* items;
};

struct UiStyle{
    glm::vec4 bg, panelBg;
    glm::vec4 barColor;
    glm::vec4 idle, hot, active;
    glm::vec4 text;
};


struct Context{
    String8 label;
    uint32_t panelIdx;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 barPos;
    glm::vec2 barSize;
    glm::vec2 ctxPos;
    glm::vec2 ctxSize;
    float padding;
    float spacing;
    float border;
    bool sameLine;
};

struct UiState{
    glm::vec2 mousePos;
    uint32_t hover;
    uint32_t active;

    Font f;
    UiStyle style;
    Context c[3];
    uint16_t ctxCount;

    UiRenderCommandArray renderCommands;
};

//TODO:change it's just a stub
uint32_t hashId(String8 label){
    int c;
    uint32_t result = 5381; //seed
    for(int i = 0; i < label.size; i++){
        //char* c = label.str[i];
        c = label.str[i];
        result = ((result << 5) + result) + c;
    }
    return result;
}

//static UiState state = {};
UiState* state;

UiState* initUi(Arena* arena){
    state = arenaAllocStruct(arena, UiState);
    state->hover = 0;
    state->active = 0;
    state->f = loadFont("Roboto-Regular", 24);
    state->ctxCount = 0;

    state->renderCommands = {};
    state->renderCommands.items = arenaAllocArray(arena, UiRenderCommand, MAX_RENDER_COMMANDS);
    state->renderCommands.lenght = 0;

    //default style
    state->style.active      =  glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    state->style.bg          = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    state->style.hot         = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    state->style.idle        = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    state->style.panelBg     = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    state->style.text        = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    state->style.barColor    = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    return state;
}

bool aabb(glm::vec2 mousePos, glm::vec2 widgetPos, glm::vec2 widgetSize){
    return (mousePos.x >= widgetPos.x && mousePos.x <= widgetPos.x + widgetSize.x &&
            mousePos.y >= widgetPos.y && mousePos.y <= widgetPos.y + widgetSize.y);
}

void setStyle(UiState* ctx, const UiStyle* style){
    state->style = *style;
}


//void drawTextBox(Arena* a, glm::vec2 pos, glm::vec2 size){
//    TempArena tmp = getTempArena(a);
//    Font* f = getFont("Roboto-Regular");
//    renderDrawFilledRect(pos, size, 0, {0.3,0.3,0.3,1});
//    String8 text;
//    if(aabb(mousePos, pos, size)){
//        if(isJustPressed(KEYS::A)){
//            text = pushString8F(tmp.arena, "%c", 'a');
//        }
//    }
//    String8 result = pushString8F(a, "%S", text);
//    renderDrawText2D(&f, result.str, pos, 1);
//    releaseTempArena(tmp);
//}

void advanceContext(Context* c, glm::vec2 pos ,glm::vec2 size){
    c->ctxPos.x += pos.x;
    c->ctxPos.y -= pos.y;
    c->ctxSize.y += size.y;
    c->ctxSize.x = glm::max(c->ctxSize.x, c->barSize.x);
    c->ctxSize.x = glm::max(c->ctxSize.x, size.x);
    c->ctxSize.y = glm::max(c->ctxSize.y, size.y);
}

bool button(String8 label){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
    bool result = false;
    Context* c = &state->c[state->ctxCount - 1];
    uint32_t id = hashId(label);

    float tWidth = calculateTextWidth(&state->f, label.str, 1);
    float tHeight = calculateTextHeight(&state->f, label.str, 1);
    glm::vec2 size = {tWidth, tHeight};

    glm::vec4 color = state->style.bg;

    advanceContext(c, {0, size.y + c->spacing}, {size.x,size.y + c->spacing});

    LOGINFO("%u", state->active);
    if(aabb(state->mousePos, c->ctxPos, size)){
        color = state->style.hot;
        state->hover = id;
    }
    if(state->active == id){
        if(isMouseButtonRelease(MOUSE_BUTTON_LEFT)){
            if(state->hover == id) result = true;
            state->active = 0;
        }
        color = state->style.active;
    }else if(state->hover == id){
        color = state->style.hot;
        if(isMouseButtonJustPressed(MOUSE_BUTTON_LEFT)){
            state->active = id;
        }
    }

    UiRenderCommand command = {};
    command.type    = UI_RECTANGLE;
    command.pos     = c->ctxPos;
    command.size    = size;
    command.color   = color;
    state->renderCommands.items[state->renderCommands.lenght++] = command;
    command.type    = UI_TEXT;
    command.label   = label;
    command.scale   = 1.0f;
    command.pos     = c->ctxPos;
    command.size    = {0,0};
    command.color   = state->style.text;
    state->renderCommands.items[state->renderCommands.lenght++] = command;

    advanceContext(c, {0, c->spacing}, {0, c->spacing});

    //renderDrawFilledRect(c->ctxPos, size, 0, color, UI_LAYER + state->ctxCount);
    //renderDrawText2D(&state->f, label.str, c->ctxPos, 1, state->style.text, UI_LAYER + state->ctxCount);

    return result;
}

bool buttonEx(String8 label, glm::vec2 pos, glm::vec2 size){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
    Context* c = &state->c[state->ctxCount - 1];
    uint32_t id = hashId(label);

    glm::vec4 color = state->style.bg;
    glm::vec2 screenPos = {pos.x, getScreenSize().y - pos.y - size.y};
    if(aabb(state->mousePos, screenPos, size)){
        color = state->style.hot;
        state->hover = id;
    }
    if(state->hover == id){
        if(isMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            color = state->style.active;
            state->active = id;
        }
    }
    renderDrawFilledRect(screenPos, size, 0, color);
    renderDrawText2D(&state->f, label.str, screenPos, 1, state->style.text);
    return state->active == id;
}

bool checkBox(String8 label, bool *value){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
    Context* c = &state->c[state->ctxCount - 1];
    uint32_t id = hashId(label);

    float tWidth = calculateTextWidth(&state->f, label.str, 1);
    float tHeight = calculateTextHeight(&state->f, label.str, 1);
    glm::vec2 pos = {0,0};
    glm::vec2 textPos = pos;
    glm::vec2 size = {tHeight, tHeight}; //checkbox size equalt to text height
    
    pos = {c->ctxPos.x + tWidth, c->ctxPos.y - tHeight};
    textPos = {c->ctxPos.x, c->ctxPos.y - size.y};
    c->ctxSize.y -= size.y + c->padding;


    glm::vec4 color = state->style.bg;
    if(aabb(state->mousePos, pos, size)){
        state->hover = id;
    }
    if(state->hover == id){
        if(isMouseButtonJustPressed(MOUSE_BUTTON_LEFT)){
            color = state->style.active;
            state->active = id;
            *value = !(*value);
        }
    }
    renderDrawFilledRect(pos, size, 0, state->style.bg);
    if(*value){
        color = state->style.active;
        renderDrawFilledRect(pos + size * 0.5f * 0.5f, size * 0.5f, 0, color);
    }
    renderDrawText2D(&state->f, label.str, textPos, 1);
    return state->active == id;
}

bool slider(Arena* a, String8 label, float* value, float min, float max){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
    Context* c = &state->c[state->ctxCount - 1];
    uint32_t id = hashId(label);

    String8 text = pushString8F(a, "%.1f", *value);
    float tWidth = calculateTextWidth(&state->f, text.str, 1);
    float tHeight = calculateTextHeight(&state->f, text.str, 1);

    float labelWidth = calculateTextWidth(&state->f, label.str, 1);
    glm::vec2 labelPos;

    glm::vec2 size = {c->ctxSize.x , tHeight};
    glm::vec2 pos = {0,0};
    glm::vec2 sSize = {size.x * 0.1f , size.y};
    glm::vec2 sPos = {0,0};
    c->ctxPos.y -= size.y;
    c->ctxSize.y -= size.y;
    labelPos = {c->ctxPos.x, c->ctxPos.y};
    pos = {c->ctxPos.x + labelWidth, c->ctxPos.y };
    sPos = {pos.x + ((*value / max) * (size.x)) - sSize.x * 0.5f, pos.y };
    c->ctxSize.y -= size.y + c->padding;

    glm::vec4 color = state->style.bg;
    if(aabb(state->mousePos, pos, size)){
        state->hover = id;
        color = state->style.hot;
    }

    if(state->hover == id){
        if(isMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            state->active = id;
            sPos.x = state->mousePos.x - sSize.x * 0.5f;
            float norm = ((sPos.x - pos.x + sSize.x * 0.5f) / size.x) * max;
            *value = glm::clamp(norm, min, max);
            color = state->style.active;
        }
    }
    glm::vec2 textPos = {pos.x + size.x * 0.5f - tWidth * 0.5f, pos.y};

    renderDrawText2D(&state->f, label.str, {labelPos.x, labelPos.y}, 1);
    renderDrawText2D(&state->f, text.str, {textPos.x, textPos.y}, 1);
    renderDrawFilledRect({pos.x, pos.y}, size, 0, color);
    renderDrawFilledRect({sPos.x, sPos.y}, sSize, 0, color);

    return state->active == id;
}

bool sliderInt(Arena* a, String8 label, int* value, int min, int max){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
    Context* c = &state->c[state->ctxCount - 1];
    uint32_t id = hashId(label);

    String8 text = pushString8F(a, "%d", *value);
    float tWidth = calculateTextWidth(&state->f, text.str, 1);
    float tHeight = calculateTextHeight(&state->f, text.str, 1);

    float labelWidth = calculateTextWidth(&state->f, label.str, 1);
    glm::vec2 labelPos;

    glm::vec2 size = {c->ctxSize.x - labelWidth - c->padding, tHeight};
    glm::vec2 pos = {0,0};
    glm::vec2 sSize = {size.x * 0.1f , size.y};
    glm::vec2 sPos = {0,0};
    c->ctxPos.y -= size.y;
    c->ctxSize.y -= size.y;
    labelPos = {c->ctxPos.x, c->ctxPos.y};
    pos = {c->ctxPos.x + labelWidth, c->ctxPos.y };
    sPos = {pos.x + (((float)*value / max) * (size.x)) - sSize.x * 0.5f, pos.y };
    c->ctxSize.y -= size.y + c->padding;

    glm::vec4 color = state->style.bg;
    if(aabb(state->mousePos, pos, size)){
        state->hover = id;
        color = state->style.hot;
    }

    if(state->hover == id){
        if(isMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            state->active = id;
            sPos.x = state->mousePos.x - sSize.x * 0.5f;
            int norm = glm::floor(((sPos.x - pos.x + sSize.x * 0.5f) / size.x) * max);
            *value = (int)glm::clamp(norm, min, max);
            color = state->style.active;
        }
    }
    glm::vec2 textPos = {pos.x + size.x * 0.5f - tWidth * 0.5f, pos.y};

    renderDrawText2D(&state->f, label.str, {labelPos.x, labelPos.y}, 1);
    renderDrawText2D(&state->f, text.str, {textPos.x, textPos.y}, 1);
    renderDrawFilledRect({pos.x, pos.y}, size, 0, color);
    renderDrawFilledRect({sPos.x, sPos.y}, sSize, 0, color);

    return state->active == id;
}

void beginPanel(String8 title, glm::vec2 pos, glm::vec2 size){
    ASSERT(state->ctxCount < 16, "UI context stack overflow");
    Context c;
    c.label = title;
    c.pos = {pos.x, getScreenSize().y - pos.y};
    //c.pos = {pos.x, pos.y};
    c.size = size;
    c.size = {0,0};
    c.padding = 5;
    c.spacing = 5;
    c.ctxPos = c.pos;
    c.ctxSize = {0,0};
    float textHeight = calculateTextHeight(&state->f, title.str, 1);
    float textWidth = calculateTextWidth(&state->f, title.str, 1);
    //c.ctxSize = {textWidth, textHeight};
    c.barSize = {textWidth + c.spacing, textHeight + c.spacing};
    c.barPos = {c.pos.x, c.pos.y - c.barSize.y};
    //c.ctxPos.y -= textHeight;
    advanceContext(&c, {c.spacing,c.barSize.y}, {c.barSize.x, c.barSize.y});
    //c.barColor = {0.0f, 0.0f, 1.0f, 1.0f};
    //renderDrawFilledRect(c.pos, size, 0, color);
    //advanceContext(&c, {c.spacing, -size.y + c.barSize.y + c.spacing + textHeight});
    //advanceContext(&c, {c.spacing, 0}, {c.spacing, 0});


    state->renderCommands.lenght++;
    c.panelIdx = state->renderCommands.lenght-1;
    //advanceContext(&c, {0, c.barSize.y + c.spacing}, {c.barSize.x, c.barSize.y});
    ////Content
    //renderDrawFilledRect({c.pos.x , c.ctxPos.y}, c.ctxSize, 0, color, UI_LAYER + state->ctxCount);
    ////TitleBar
    //renderDrawFilledRect(c.barPos, c.barSize, 0, barColor, UI_LAYER + state->ctxCount);
    //renderDrawText2D(&state->f, title.str, c.barPos, 1, state->style.text, UI_LAYER + state->ctxCount);


    state->c[state->ctxCount++] = c;
}

void endPanel(){
    ASSERT(state->ctxCount > 0, "UI context stack underflow (mismatched endPanel)");
    Context* c = &state->c[state->ctxCount - 1];
    UiRenderCommand command = {};
    command.type    = UI_RECTANGLE;
    command.pos     = c->barPos;
    command.size.x  = glm::max(c->barSize.x, c->ctxSize.x);
    command.size.y  = c->barSize.y;
    command.color   = state->style.barColor;
    state->renderCommands.items[state->renderCommands.lenght++] = command;
    command.type    = UI_RECTANGLE;
    command.pos     = {c->pos.x, c->ctxPos.y};
    command.size    = c->ctxSize;
    command.color   = state->style.panelBg;
    state->renderCommands.items[c->panelIdx] = command;
    command.type    = UI_TEXT;
    command.label   = c->label;
    command.scale   = 1.0f;
    command.pos     = c->barPos;
    command.size    = {0,0};
    command.color   = state->style.text;
    state->renderCommands.items[state->renderCommands.lenght++] = command;
    state->ctxCount--;
}

void begin(){
    ASSERT(state->ctxCount < 16, "UI context stack overflow");
    state->hover = 0;
    Context c;
    c.pos = {0,getScreenSize().y};
    c.size = getScreenSize();
    c.padding = 10;
    c.ctxSize.x = c.size.x;
    c.ctxSize.y = c.size.y;
    state->c[state->ctxCount++] = c;
    state->mousePos = getMousePos();
}

void end(){
    ASSERT(state->ctxCount > 0, "UI context stack underflow (mismatched end)");
    state->ctxCount--;
    state->renderCommands.lenght = 0;
}

void drawHud(GameState* gs, Arena* a){
    TempArena tmp = getTempArena(a);
    glm::vec2 screenSize = getScreenSize();

    glm::vec2 panelSize = {250,250};
    //glm::vec2 panelPos = {screenSize.x - 300, screenSize.y * 0.5f - panelSize.y * 0.5f};
    glm::vec2 panelPos = {10,10};
    //LOGINFO("%f, %f | %f, %f | %f, %f", mousePos.x, mousePos.y, panelPos.x, panelPos.y, screenSize.x, screenSize.y);
    float padding = 10;
    int columns = 5;
            static bool t = false;
            static bool showPanel = false;
    begin();
    beginScene(NO_DEPTH);
    //clearColor(0,0,0,1);
        //Right Panel
        //drawRightPanel(tmp.arena, panelPos, panelSize, padding, columns);
        //columns = 5;
        //beginPanel(String8Lit("Test"), {0,0}, panelSize);
        //if(button(String8Lit("OPS"))){
        //beginPanel(String8Lit("Test"), {300,300}, {300,300});
        //    slider(tmp.arena, String8Lit("radius"), &gs->radius, 1, 50);
        //    sliderInt(tmp.arena, String8Lit("iterations"), &gs->iterations, 1, 1000);
        //endPanel();

        //}
        //button(String8Lit("OPS2"));
        //button(String8Lit("OPS3"));
        beginPanel(String8Lit("Test"), panelPos, panelSize);
            if(button(String8Lit("O"))){
                showPanel = !showPanel;
            }
            if(showPanel){
                beginPanel(String8Lit("Test"), {100,50}, panelSize);
                    button(String8Lit("OPS2"));
                    button(String8Lit("OPS2sdfasdfasdfasd"));
                    button(String8Lit("OPS3"));
                endPanel();
            }
            button(String8Lit("O"));
            button(String8Lit("O"));
        endPanel();
        //beginPanel(String8Lit("Test2"), {10,10}, {200,200}, CTX_HORIZONTAL_ALIGNMENT);
        //    button(String8Lit("OPS5"));
        //    button(String8Lit("OPS6"));
        //endPanel();
        //beginPanel(String8Lit("Test"), panelPos, panelSize);
        //    slider(tmp.arena, String8Lit("radius"), &gs->radius, 1, 50);
        //    sliderInt(tmp.arena, String8Lit("iterations"), &gs->iterations, 1, 1000);
        //    checkBox(String8Lit("checkBox"), &t);
        //endPanel();

        //panelPos = {100,100};
        //panelSize = {300, 100};
        //beginPanel(String8Lit("Test 2"), panelPos, panelSize, CTX_HORIZONTAL_ALIGNMENT);
        //    button(String8Lit("ciao"));
        //    button(String8Lit("ciao2"));
        //    checkBox(String8Lit("checkBox"), &t);
        //    slider(tmp.arena, String8Lit("radius"), &gs->radius, 1, 50);
        //endPanel();

        //drawTextBox(a, {500, 100}, {100,50});
    //for(int i = state.renderCommands.lenght-1; i >= 0; i--){
    for(uint32_t i = 0; i < state->renderCommands.lenght; i++){
        UiRenderCommand* command = &state->renderCommands.items[i];
        switch(command->type){
            case UI_RECTANGLE:{
                LOGINFO("%f,%f,%f,%f", command->color.r, command->color.g, command->color.b, command->color.a);
                renderDrawFilledRect(command->pos, command->size, 0, command->color);
                break;
            }
            case UI_TEXT:{
                renderDrawText2D(&state->f, command->label.str, command->pos, command->scale, command->color);
                break;
            }
        }
    }
    endScene();
    end();

    //horizontal layout
    //panelPos = {screenSize.x - 300, 50};
    //panelSize = {250,900};

    releaseTempArena(tmp);
}

GAME_API void gameStart(Arena* gameArena, EngineState* engineState){
    if(gameArena->index > 0){
        return;
    }
    GameState* gs = arenaAllocStruct(gameArena, GameState);
    gs->arena = gameArena;
    gs->restart = false;
    gs->mainCamera = createCamera(-640.0f / 2, 640.0f / 2, -320.0f / 2, 320.0f / 2);
    setActiveCamera(&gs->mainCamera);

    gs->iterations = 50;
    gs->radius = 50;
    gs->whiteTexture = getWhiteTexture();
    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    gs->uiState = initUi(gameArena);
    //---------------------------------------
    gs->f = loadFont("Roboto-Regular", 24);
    gs->texture = loadTexture("awesome");
}

GAME_API void gameRender(Arena* gameArena, EngineState* engine, float dt){}

GAME_API void gameUpdate(Arena* gameArena, EngineState* engineState, float dt){
    GameState* gs = (GameState*)gameArena->memory;
    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    state = gs->uiState;
    //---------------------------------------

    beginScene();
        beginMode2D(gs->mainCamera);
            clearColor(0, 0, 0, 1);
            renderDrawText2D(&gs->f, "ciao", {0,0}, 1);
            renderDrawRect({10,10}, {100,100}, {1,1,1,1});
            renderDrawQuad2D({0,0},{100,100}, 0, &gs->texture);
            //renderDrawFilledRect({0, 0}, {100,100}, 0, {1,1,1,1});
            //renderDrawQuad2D({0,0}, {100,100}, 0, &gs->whiteTexture);
            //renderDrawCirclePro({100,100},100,{0,0},{1,1,1,1}, 1);
        endMode2D();
            renderDrawQuad2D({0,0},{100,100}, 0, &gs->texture);
    endScene();
    drawHud(gs, gameArena);
}

GAME_API void gameStop(Arena* gameArena, EngineState* engine){
}