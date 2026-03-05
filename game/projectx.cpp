#include "projectx.hpp"
#include "customcomponents.hpp"

#define ASSERT(condition, msg) \
    do { \
        if(!(condition)) { \
            LOGERROR("ASSERT FAILED: %s | %s:%d", msg, __FILE__, __LINE__); \
            __builtin_debugtrap(); \
        } \
    } while(0)



// UI CODE --------------------------------------------
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
    uint32_t panelIdx;   // index into renderCommands where the panel background is reserved
    glm::vec2 pos;       // anchor point of the panel (screen space, y-up from bottom-left)
    glm::vec2 size;      // final computed size, filled in by endPanel
    glm::vec2 barPos;    // position of the title bar
    glm::vec2 barSize;   // size of the title bar
    glm::vec2 ctxPos;    // cursor: where the next widget will be placed (moves down each widget)
    glm::vec2 ctxSize;   // accumulated content area size (grows as widgets are added)
    float padding;
    float spacing;       // gap between widgets
    float border;
    // lineEnd holds the right/top edge of the last placed widget.
    // sameLine swaps ctxPos <-> lineEnd to continue on the same line instead of going down.
    glm::vec2 lineEnd;
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
    for(uint64_t i = 0; i < label.size; i++){
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

// Swap ctxPos and lineEnd so the next widget is placed to the right of the previous one
// instead of below it. advanceContext always writes the "next line" position into lineEnd,
// so swapping restores the horizontal continuation point.
void sameLine(){
    Context* c = &state->c[state->ctxCount-1];

    glm::vec2 temp;
    temp = c->ctxPos;
    c->ctxPos.x = c->lineEnd.x;
    c->lineEnd.x = temp.x;
    temp = c->ctxPos;
    c->ctxPos.y = c->lineEnd.y;
    c->lineEnd.y = temp.y;
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

// Move the cursor down by the widget height and update the accumulated content size.
// Also records lineEnd = right/top edge of this widget, which sameLine uses to
// continue horizontally. ctxPos.x is reset to the panel left edge after each widget.
// NOTE: always advances Y regardless of sameLine — mixed-height same-line rows will drift.
void advanceContext(Context* c, glm::vec2 size){
    c->ctxPos.y -= size.y;
    c->ctxSize.y = c->pos.y - c->ctxPos.y;
    // width is the max right edge seen so far (lineEnd.x tracks the widest row)
    c->ctxSize.x = c->lineEnd.x - c->pos.x;
    c->ctxSize.x = glm::max(c->ctxSize.x, c->barSize.x);

    c->lineEnd.x = c->ctxPos.x + size.x; // right edge of this widget
    c->lineEnd.y = c->ctxPos.y + size.y; // top edge of this widget (for sameLine Y restore)
    c->ctxPos.x = c->pos.x;              // reset X to panel left
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
    // ctxPos is the cursor top edge; widget renders from (ctxPos.y - size.y) upward (y-up coords)
    glm::vec2 pos;
    pos = {c->ctxPos.x , c->ctxPos.y - size.y };

    if(aabb(state->mousePos, pos, size)){
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
    command.pos     = pos;
    command.size    = size;
    command.color   = color;
    state->renderCommands.items[state->renderCommands.lenght++] = command;
    command.type    = UI_TEXT;
    command.label   = label;
    command.scale   = 1.0f;
    command.pos     = pos;
    command.size    = {0,0};
    command.color   = state->style.text;
    state->renderCommands.items[state->renderCommands.lenght++] = command;

    //advance only on y direction
    //branch it when adding horizontal layout
    advanceContext(c, {size.x , size.y });
    return result;
}

bool buttonEx(String8 label, glm::vec2 pos, glm::vec2 size){
    if(state->ctxCount == 0){
        LOGERROR("No context");
        return false;
    }
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
    c.size = size;
    c.size = {0,0}; //assign a default size???
    c.padding = 5;  //should be elsewhere??
    c.spacing = 5;  //should be elsewhere??
    c.ctxPos = c.pos;
    c.lineEnd = c.ctxPos;
    c.ctxSize = {0,0};
    float textHeight = calculateTextHeight(&state->f, title.str, 1);
    float textWidth = calculateTextWidth(&state->f, title.str, 1);
    c.barSize = {textWidth, textHeight};
    c.barPos = {c.pos.x, c.pos.y - c.barSize.y};
    state->renderCommands.lenght++;
    c.panelIdx = state->renderCommands.lenght-1;
    advanceContext(&c, {c.barSize.x, c.barSize.y});
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
    glm::vec2 panelPos = {10,10};
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
            sameLine();
            button(String8Lit("OP"));
            sameLine();
            button(String8Lit("OPS"));
            sameLine();
            button(String8Lit("OPSS"));
            sameLine();
            button(String8Lit("OPSSS"));
            button(String8Lit("OPSSSS"));
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
    //for(int i = state->renderCommands.lenght-1; i >= 0; i--){
    for(uint32_t i = 0; i < state->renderCommands.lenght; i++){
        UiRenderCommand* command = &state->renderCommands.items[i];
        switch(command->type){
            case UI_RECTANGLE:{
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

    releaseTempArena(tmp);
}

// --------------------------------------------


void systemRenderEcs(){
    EntityArray entities = view(ECS_TYPE(SpriteComponent), ECS_TYPE(TransformComponent));
    for(size_t i = 0; i < entities.count; i++){
        Entity e = entities.entities[i];
        SpriteComponent* sprite = getComponent(e, SpriteComponent);
        TransformComponent* transform = getComponent(e, TransformComponent);
        ASSERT((sprite && transform), "Ecs renderer failed, null component");
        Rect rect = {{transform->position.x, transform->position.y}, sprite->size};
        renderDrawQuadPro(transform->position, sprite->size, transform->rotation,
                            rect, {0.5f, 0.5f}, sprite->texture, sprite->color,
                            sprite->ySort, sprite->ySortOffset);
    }
}

void systemGravity(GameState* gs, float dt){
    static float radius = 50;
    static float gravity = 90;
    static float softening = 5;
    EntityArray entities = view(ECS_TYPE(TransformComponent),    ECS_TYPE(VelocityComponent),
                                ECS_TYPE(AccelerationComponent), ECS_TYPE(AsteroidTag),
                                ECS_TYPE(MassComponent));
    for(size_t i = 0; i < entities.count; i++){
        Entity e = entities.entities[i];
        TransformComponent* transform = getComponent(e, TransformComponent);
        VelocityComponent* velocity = getComponent(e, VelocityComponent);
        AccelerationComponent* acceleration = getComponent(e, AccelerationComponent);
        MassComponent* mass = getComponent(e, MassComponent);
        for(size_t j = i+1; j < entities.count; j++){
            Entity e2 = entities.entities[j];
            TransformComponent* transform2 = getComponent(e2, TransformComponent);
            AccelerationComponent* acceleration2 = getComponent(e2, AccelerationComponent);
            MassComponent* mass2 = getComponent(e2, MassComponent);
            ASSERT((transform2 && transform2 && mass2), "Ecs gravity failed, null component");
            if(glm::distance(transform->position, transform2->position) <= radius){
                glm::vec2 diff = glm::vec2(transform2->position) - glm::vec2(transform->position);
                float dist = glm::sqrt(glm::length(diff) + softening * softening);
                float force = gravity * mass->v * mass2->v / (dist * dist);
                glm::vec2 dir = glm::normalize(diff);
                //glm::vec2 force = glm::normalize(diff);
                //acceleration2->a -= glm::normalize(diff) * gravity * mass->v;
                //acceleration->a  += glm::normalize(diff) * gravity * mass2->v;
                acceleration2->a -= dir * (force / mass->v );
                acceleration->a  += dir * (force / mass2->v);
            }
        }
    }

}

void systemMovement(float dt){
    EntityArray entities = view(ECS_TYPE(TransformComponent), ECS_TYPE(VelocityComponent), ECS_TYPE(AccelerationComponent));
    for(size_t i = 0; i < entities.count; i++){
        Entity e = entities.entities[i];
        TransformComponent* transform = getComponent(e, TransformComponent);
        VelocityComponent* velocity = getComponent(e, VelocityComponent);
        AccelerationComponent* acceleration = getComponent(e, AccelerationComponent);
        ASSERT((transform && velocity && acceleration), "Ecs movement failed, null component");
        velocity->vel += acceleration->a * dt;
        acceleration->a = {0, 0};
        transform->position.x += velocity->vel.x * dt;
        transform->position.y += velocity->vel.y * dt; 
        //LOGINFO("%f, %f", velocity->vel.x, velocity->vel.y);
        //velocity->vel = {0, 0};// remove the reset to have a character which doesn't have breaks
    }
}

void systemGameInput(){
    EntityArray entities = view(ECS_TYPE(PlayerTag));
    for(size_t i = 0; i < entities.count; i++){
        Entity e = entities.entities[i];
        VelocityComponent* velocity = getComponent(e, VelocityComponent);
        AccelerationComponent* acceleration = getComponent(e, AccelerationComponent);
        ASSERT((velocity), "Ecs movement failed, null component");
        //velocity->vel = {0,0};
        acceleration->a = {0,0};
        if(isPressed(KEYS::D)) acceleration->a.x = +1.0f;
        if(isPressed(KEYS::A)) acceleration->a.x = -1.0f;
        if(isPressed(KEYS::W)) acceleration->a.y = +1.0f;
        if(isPressed(KEYS::S)) acceleration->a.y = -1.0f;
        //LOGINFO("%f, %f", acceleration->a.x, acceleration->a.y);
    }
}

void systemSpawnAsteroids(GameState* gs, float dt){
    //TODO: change rand with a better random function
    static float rate;
    float rateSpawn = 1.0f;
    if(rate >= rateSpawn){
        glm::vec2 spawnRange = gs->gameSize;
        int baseSize = 10;
        int maxSize = 20;
        float baseSpeed = 10.0f;
        float size = (rand() % maxSize) + baseSize;
        Entity e = createEntity();
        TransformComponent transform = {};
        glm::vec3 spawnPos = {((((float)rand() / RAND_MAX) - 0.5f) * spawnRange.x), ((((float)rand() / RAND_MAX) - 0.5f) * spawnRange.y), 0.0f};
        glm::vec3 playerPos = getComponent(gs->player, TransformComponent)->position;
        glm::vec2 dir = playerPos - spawnPos;
        transform.position = spawnPos;
        pushComponent(e, TransformComponent, &transform);
        SpriteComponent sprite = {};
        sprite.color = {1.0f,1.0f,1.0f,1};
        sprite.layer = 1;
        sprite.size = {size, size};
        sprite.texture = &gs->gameTextures[PLAYER_TEXTURE];
        sprite.visible = true;
        sprite.ySort = true;
        sprite.ySortOffset = 0;
        pushComponent(e, SpriteComponent, &sprite);
        VelocityComponent velocity = {};
        velocity.vel = glm::normalize(dir) * baseSpeed;
        pushComponent(e, VelocityComponent, &velocity);
        AccelerationComponent acceleration = {};
        //acceleration.a = glm::normalize(dir) * baseSpeed;
        acceleration.a = {10,10};
        pushComponent(e, AccelerationComponent, &acceleration);
        MassComponent mass = {};
        mass.v = size;
        //mass.v = 1;
        pushComponent(e, MassComponent, &mass);
        AsteroidTag asteroidTag = {};
        pushComponent(e, AsteroidTag, &asteroidTag);
        rate = 0;
    }
    rate += dt;
}

void systemDestroyAsteroids(GameState* gs){
    EntityArray entities = view(ECS_TYPE(TransformComponent), ECS_TYPE(AsteroidTag));
    for(size_t i = 0; i < entities.count; i++){
        Entity e = entities.entities[i];
        TransformComponent* transform = getComponent(e, TransformComponent);
        ASSERT((transform), "Ecs movement failed, null component");
        if( transform->position.x >=  gs->mainCamera.width  * 2  || 
            transform->position.x <= -gs->mainCamera.width  * 2  ||
            transform->position.y <= -gs->mainCamera.height * 2  ||
            transform->position.y >=  gs->mainCamera.height * 2){
                removeEntity(e);
            }
    }

}

GAME_API void gameStart(Arena* gameArena){
    if(gameArena->index > 0){
        return;
    }
    GameState* gs = arenaAllocStruct(gameArena, GameState);
    gs->arena = gameArena;
    gs->restart = false;
    gs->gameSize = {640, 320};
    gs->mainCamera = createCamera(-gs->gameSize.x / 2, gs->gameSize.x / 2, -gs->gameSize.y / 2, gs->gameSize.y/ 2);
    setActiveCamera(&gs->mainCamera);

    srand(1);

    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    gs->uiState = initUi(gameArena);
    //---------------------------------------
    gs->f = loadFont("Roboto-Regular", 24);
    gs->texture = loadTexture("awesome");
    gs->finalTexture = loadRenderTexture(640, 320);

    gs->gameTextures[PLAYER_TEXTURE] = loadWhiteTexture();

    importCustomComponentModule();

    //player creation
    Entity e = createEntity();
    gs->player = e;
    TransformComponent transform;
    transform.position = {0,0,0};
    transform.rotation = {0,0,0};
    transform.scale    = {1,1,1};
    pushComponent(e, TransformComponent, &transform);
    SpriteComponent sprite;
    sprite.color = {1,1,1,1};
    sprite.layer = 1;
    sprite.size = {10, 10};
    sprite.texture = &gs->gameTextures[PLAYER_TEXTURE];
    sprite.visible = true;
    sprite.ySort = true;
    sprite.ySortOffset = 0;
    pushComponent(e, SpriteComponent, &sprite);
    VelocityComponent velocity;
    velocity.vel = {0.0f, 0.0f};
    pushComponent(e, VelocityComponent, &velocity);
    AccelerationComponent acceleration = {};
    acceleration.a = {0.0f, 0.0f};
    pushComponent(e, AccelerationComponent, &acceleration);
    PlayerTag playerTag;
    pushComponent(e, PlayerTag, &playerTag);
}

GAME_API void gameRender(Arena* gameArena, float dt){}

GAME_API void gameUpdate(Arena* gameArena, float dt){
    GameState* gs = (GameState*)gameArena->memory;
    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    state = gs->uiState;
    //---------------------------------------

    //reimport modules for dll reload
    importCustomComponentModule();

    systemSpawnAsteroids(gs, dt);

    //sytems updates
    systemGameInput();
    systemGravity(gs, dt);
    systemMovement(dt);
    systemDestroyAsteroids(gs);
    //----------------

    //render the game into the texture world
    beginTextureMode(&gs->finalTexture);
        beginMode2D(gs->mainCamera);
            clearColor(0.5f,0.5f,0.5f,1);
            systemRenderEcs();
        endMode2D();
    endTextureMode();

    //World drawing on texture to never lose the initial resolution
    Rect rect;
    rect.pos = {0,0};
    rect.size = {640, 320};
    beginScene();
        clearColor(0, 0, 0, 1);
        renderDrawQuadPro2D(getScreenSize() * 0.5f, {rect.size.x, -rect.size.y},
                            0, rect, {0.5f,0.5f}, &gs->finalTexture.texture);
    endScene();

    //Ui Code -------------------------------------
    //drawHud(gs, gameArena);
}

GAME_API void gameStop(Arena* gameArena){
}