#include "projectx.hpp"

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
    Color color;
};

struct UiRenderCommandArray{
    uint32_t lenght;
    UiRenderCommand* items;
};

enum UiLayoutType{
    LAYOUT_VERTICAL,
    LAYOUT_HORIZONTAL,
};

struct UiLayout{
    UiLayoutType type;
    glm::vec2 size;
    uint32_t items;
};

struct UiStyle{
    Color bg, panelBg;
    Color barColor;
    Color idle, hot, active;
    Color text;
};


struct Context{
    String8 label;
    UiLayout* layout;
    uint32_t panelIdx;   // index into renderCommands where the panel background is reserved
    glm::vec2 pos;       // anchor point of the panel (screen space, y-up from bottom-left)
    glm::vec2 barPos;    // position of the title bar
    glm::vec2 barSize;   // size of the title bar
    glm::vec2 ctxPos;    // bottom-left of the content area (used for panel background rect)
    glm::vec2 ctxSize;   // accumulated content area size (grows as widgets are added)
    glm::vec2 cursorPos; // where the next widget will be placed
    float padding;
    float spacing;       // gap between widgets
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
    state->style.active      = ColorFromRGBA(255,   0,   0, 255);
    state->style.bg          = ColorFromRGBA(128, 128, 128, 255);
    state->style.hot         = ColorFromRGBA(255, 255,   0, 255);
    state->style.idle        = ColorFromRGBA(  0,   0,   0, 255);
    state->style.panelBg     = ColorFromRGBA( 76,  76,  76, 255);
    state->style.text        = ColorFromRGBA(255, 255, 255, 255);
    state->style.barColor    = ColorFromRGBA(  0,   0, 255, 255);
    return state;
}

bool aabb(glm::vec2 mousePos, glm::vec2 widgetPos, glm::vec2 widgetSize){
    return (mousePos.x > widgetPos.x && mousePos.x < widgetPos.x + widgetSize.x &&
            mousePos.y > widgetPos.y && mousePos.y < widgetPos.y + widgetSize.y);
}

void setStyle(UiState* ctx, const UiStyle* style){
    state->style = *style;
}

// Advance the cursor after placing a widget.
// With no layout: moves Y down, resets X to panel left (vertical stacking).
// With LAYOUT_HORIZONTAL: moves X right by cell width; wraps to next row after `items` cells.
// With LAYOUT_VERTICAL: moves Y down by cell height.
void advanceContext(Context* c, glm::vec2 size){
    if(c->layout){
        if(c->layout->type == LAYOUT_HORIZONTAL){
            float cellW = c->layout->size.x / (float)c->layout->items;
            float cellH = c->layout->size.y;
            c->cursorPos.x += cellW;
            //c->cursorPos.y = glm::min(cellH, c->cursorPos.y);
            // wrap: if next cell would exceed layout width, go to next row
            //if(c->cursorPos.x - c->pos.x >= c->layout->size.x - cellW` * 0.5f){
            //    c->cursorPos.x = c->pos.x;
            //    c->cursorPos.y -= cellH + c->spacing;
            //}
            c->ctxSize.x = glm::max(c->ctxSize.x, c->layout->size.x);
            c->ctxSize.y = glm::max(c->ctxSize.y, c->pos.y - c->cursorPos.y);
            c->ctxPos = {c->ctxPos.x, c->cursorPos.y - cellH};
        } else { // LAYOUT_VERTICAL
            float cellH = c->layout->size.y / (float)c->layout->items;
            c->cursorPos.y -= cellH;// + c->spacing;
            c->ctxSize.x = glm::max(c->ctxSize.x, c->layout->size.x);
            c->ctxSize.y = glm::max(c->ctxSize.y, c->pos.y - c->cursorPos.y);
        }
        c->ctxPos.y = glm::min(c->ctxPos.y, c->cursorPos.y);
        //c->ctxPos.y = glm::min(c->ctxPos.y, size.y);
    } else {
        // default vertical stacking: natural widget size
        c->cursorPos.y -= size.y;// + c->spacing;
        c->ctxSize.y = c->pos.y - c->barSize.y - c->cursorPos.y;
        c->ctxSize.x = glm::max(c->ctxSize.x, size.x);
        c->ctxSize.x = glm::max(c->ctxSize.x, c->barSize.x);
        c->cursorPos.x = c->pos.x;
        c->ctxPos.y = glm::min(c->ctxPos.y, c->cursorPos.y);
    }
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
    glm::vec2 size;
    if(c->layout){
        if(c->layout->type == LAYOUT_HORIZONTAL){
            size = {c->layout->size.x / (float)c->layout->items, c->layout->size.y};
        }else{
            size = {c->layout->size.x, c->layout->size.y / (float)c->layout->items};
        }
    } else {
        size = {tWidth, tHeight};
    }

    Color color = state->style.bg;
    glm::vec2 pos = {c->cursorPos.x, c->cursorPos.y - size.y};

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

    advanceContext(c, size);
    return result;
}

UiLayout makeLayout(UiLayoutType type, glm::vec2 size, uint32_t items){
    UiLayout layout = {};
    layout.type = type;
    layout.size = size;
    layout.items = items;
    return layout;
}

void pushLayout(UiLayout* layout){
    Context* c = &state->c[state->ctxCount-1];
    ASSERT(c, "No context in the stack");
    c->layout = layout;
}

void popLayout(){

}

void beginPanel(String8 title, glm::vec2 pos, glm::vec2 size){
    ASSERT(state->ctxCount < 16, "UI context stack overflow");
    Context c = {};
    c.label    = title;
    //c.layout   = LAYOUT_VERTICAL;
    c.pos      = {pos.x, getScreenSize().y - pos.y};
    c.padding  = 5;
    c.spacing  = 5;
    c.ctxPos   = c.pos;
    c.cursorPos = c.pos;
    c.ctxSize  = {0,0};//layout ? layout->size : glm::vec2{0, 0};
    float textHeight = calculateTextHeight(&state->f, title.str, 1);
    float textWidth  = calculateTextWidth(&state->f, title.str, 1);
    c.barSize  = {textWidth, textHeight};
    c.barPos   = {c.pos.x, c.pos.y - c.barSize.y};
    state->renderCommands.lenght++;
    c.panelIdx = state->renderCommands.lenght - 1;
    c.cursorPos.y -= c.barSize.y;// + c.spacing;
    //advanceContext(&c, c.barSize);
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
    command.pos     = c->ctxPos;
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
    clearColor(0,0,0,0);
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
            button(String8Lit("OP"));
            button(String8Lit("OPS"));
            button(String8Lit("OPSS"));
            button(String8Lit("OPSSS"));
            button(String8Lit("OPSSSS"));
        endPanel();
        UiLayout l = makeLayout(LAYOUT_HORIZONTAL, {600, 200}, 5);
        //beginPanel(String8Lit("CIAO"), {500,500}, {0,0}, &l);
        beginPanel(String8Lit("CIAO"), {500,500}, {0,0});
        pushLayout(&l);
            button(String8Lit("ADSFR"));
            button(String8Lit("ADSFR"));
            button(String8Lit("ADSFR"));
            button(String8Lit("ADSFR"));
            button(String8Lit("ADSFR"));
        popLayout();
        endPanel();
    //for(int i = state->renderCommands.lenght-1; i >= 0; i--){
    beginScene(NO_DEPTH);
    for(uint32_t i = 0; i < state->renderCommands.lenght; i++){
        UiRenderCommand* command = &state->renderCommands.items[i];
        switch(command->type){
            case UI_RECTANGLE:{
                renderDrawFilledRect(command->pos, command->size, 0, command->color);
                //renderDrawFilledRectPro(command->pos, command->size, 0, {0,1}, command->color);
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

enum Layer{
    LAYER_BG = 0,
};


glm::vec2 getMouseWorld(GameState* gs, float scale){
    //mouse world calculations
    glm::vec2 mousePos = getMousePos();
    glm::vec2 n = getMousePos() / getScreenSize();
    glm::vec2 quadSize = gs->gameSize * scale;
    glm::vec2 quadPos  = (getScreenSize() * 0.5f) - (quadSize * 0.5f); // top-left of quad
    float quadLeft = quadPos.x;
    float quadRight = quadPos.x + quadSize.x;
    float quadTop = quadPos.y + quadSize.y;
    float quadBot = quadPos.y;
    glm::vec2 botLeft = {quadBot, quadLeft};
    glm::vec2 topRight = {quadTop, quadRight};
    mousePos = glm::vec2(getMousePos().x - quadLeft, getMousePos().y - quadBot);
    n = mousePos / quadSize;
    glm::vec2 worldPos = (n * gs->gameSize) - (gs->gameSize * 0.5f);

    return worldPos;
}

//int ij(GameState* gs, int i, int j){
//    return (gs->worldGrid.size.y * j) + i;
//}

Cell* getGridCell(WorldGrid* grid, int i, int j){
    return &grid->cell[(grid->size.x * j) + i];
}

glm::vec2 gridPosToWorld(WorldGrid* grid, int i, int j){
    //glm::vec2 center = glm::vec2(i + 0.5f , j + 0.5f);
    glm::vec2 center = glm::vec2(i, j);
    return glm::vec2(center.x * grid->cellSize, center.y * grid->cellSize) - (glm::vec2(grid->size) * grid->cellSize * 0.5f);
}

glm::ivec2 worldPosToGrid(WorldGrid* grid, glm::vec2 pos){
    glm::ivec2 gridPos = glm::ivec2(glm::floor((pos + glm::vec2(grid->size) * grid->cellSize * 0.5f) / grid->cellSize));
    return gridPos;
}

glm::ivec2 bfs(WorldGrid* grid, int i, int j, int movement){
    glm::ivec2 result;
    return result;
}

GAME_API void gameStart(Arena* gameArena){
    if(gameArena->index > 0){
        return;
    }
    GameState* gs = arenaAllocStruct(gameArena, GameState);
    gs->arena = gameArena;
    gs->restart = false;
    gs->gameSize = {640, 360};
    gs->mainCamera = createCamera(-gs->gameSize.x / 2, gs->gameSize.x / 2, -gs->gameSize.y / 2, gs->gameSize.y/ 2);

    srand(1);

    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    //gs->uiState = initUi(gameArena);
    //---------------------------------------
    gs->f = loadFont("Roboto-Regular", 12);
    gs->finalTexture = loadRenderTexture(gs->gameSize.x, gs->gameSize.y);
    setTextureWrap(&gs->finalTexture.texture, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_WRAP_CLAMP_TO_EDGE);

    //Grid world
    gs->worldGrid = {};
    glm::ivec2 gridSize = {25, 15};
    gs->worldGrid.size = gridSize;
    gs->worldGrid.cell = arenaAllocArray(gs->arena, Cell, gridSize.x * gridSize.y);
    gs->worldGrid.cellSize = 16;
    for(int j = 0; j < gs->worldGrid.size.y; j++){
        for(int i = 0; i < gs->worldGrid.size.x; i++){
            //TODO initialize cells
            Cell* c = getGridCell(&gs->worldGrid, i, j);
            c->walkable = true;
        }
    }

    // Units
    gs->maxUnits = 16;
    gs->units = arenaAllocArray(gs->arena, Unit, gs->maxUnits);
    for(int i = 1; i < gs->maxUnits; i++){
        gs->units[i].type = UNIT_ENEMY;
        glm::vec2 pos = glm::vec2(rand() % gridSize.x, rand() % gridSize.y);
        gs->units[i].pos = pos;
        gs->units[i].movement = 2;

        Cell* cell = getGridCell(&gs->worldGrid, pos.x, pos.y);
        cell->walkable = false;
    }
    gs->units[0].type = UNIT_PLAYER;
    gs->units[0].pos = {0,0};
    gs->units[0].movement = 2;

    gs->gameTextures[PLAYER_TEXTURE] = loadWhiteTexture();
    gs->t = loadTexture("awesome");
}

GAME_API void gameRender(Arena* gameArena, float dt){}


GAME_API void gameUpdate(Arena* gameArena, float dt){
    GameState* gs = (GameState*)gameArena->memory;
    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    //state = gs->uiState;
    //---------------------------------------

    TempArena temp = getTempArena(gs->arena);
    float scale = 2.0f;
    glm::vec2 mouseScreen = getMousePos();
    glm::vec2 mouseWorld = getMouseWorld(gs, scale);
    glm::ivec2 mouseGrid = worldPosToGrid(&gs->worldGrid, mouseWorld);

    //render the game into the texture world
    glm::ivec2 gridSize = gs->worldGrid.size;
    float cellSize = gs->worldGrid.cellSize;
    beginScene(NO_DEPTH);
    clearColor(0.0f,0.0f,0.0f,1);
    beginTextureMode(&gs->finalTexture);
        beginMode2D(gs->mainCamera);
            renderDrawFilledRect(-(gs->gameSize / 2.0f), gs->gameSize, 0, ColorFromRGBA(0,0,128,255), LAYER_BG);
            //draw grid
            for(int j = 0; j < gridSize.y; j++){
                for(int i = 0; i < gridSize.x; i++){
                    Cell* c = getGridCell(&gs->worldGrid, i, j);
                    glm::vec2 cellPos = gridPosToWorld(&gs->worldGrid, i , j);
                    Color bgColor     = ColorFromRGBA(51, 128, 51, 255);
                    Color borderColor = ColorFromRGBA( 0,   0,  0, 255);
                    if(aabb(mouseWorld, cellPos, {16,16})){
                        renderDrawFilledRectPro(cellPos, glm::vec2(cellSize), 0, {0.0, 0.0}, ColorFromRGBA(0,0,255,255), LAYER_BG);
                    }else{

                    //renderDrawFilledRect(cellPos, glm::vec2(cellSize), 0, bgColor, LAYER_BG);
                    //renderDrawFilledRect(cellPos, glm::vec2(10), 0, ColorFromRGBA(255,0,0,255), LAYER_BG);
                    renderDrawFilledRectPro(cellPos, glm::vec2(cellSize), 0, {0.0, 0.0}, bgColor, LAYER_BG);
                    //renderDrawFilledRectPro(cellPos + 4.0f, glm::vec2(8), 0, {0.0, 0.0}, ColorFromRGBA(255,0,0,255), LAYER_BG);
                    renderDrawRect(cellPos, glm::vec2(cellSize), borderColor, LAYER_BG);
                    }
                }
            }
            Unit* player = &gs->units[0];
            glm::vec2 cellPos = gridPosToWorld(&gs->worldGrid, player->pos.x , player->pos.y);
            static bool dragging;
            static glm::ivec2 initialPos;
            //LOGINFO("%d", (mouseGrid == player->pos));
            //LOGINFO("%d", isMouseButtonJustPressed(MOUSE_BUTTON_LEFT));
                glm::ivec2 neig[200];
                bool visited[200];
                int w[200];
                for(int i = 0; i < 200; i++){
                    visited[i] = false;
                    w[i] = 0;
                    neig[i] = glm::vec2(0,0);
                }
                int count = 0;
                w[initialPos.y * 10 + initialPos.x] = 0;
                visited[initialPos.y * 10 + initialPos.x] = false;
                neig[0] = initialPos;
            if(dragging){
                int i = 0;
                int j = 1;
                while(i != j){
                    glm::ivec2 root = neig[i];
                    for(int y = root.y -1; y <= root.y + 1; y++){
                        for(int x = root.x -1; x <= root.x + 1; x++){
                            if( x >= 0 && x < gs->worldGrid.size.x &&
                                y >= 0 && y < gs->worldGrid.size.y){
                                
                                Cell* c = getGridCell(&gs->worldGrid, x, y);
                                //No diagonal neighborhoods
                                if(glm::abs(root.x - x) + glm::abs(root.y - y) > 1){
                                    continue;
                                }

                                //max depth
                                if(glm::abs(initialPos.y - y) > player->movement || glm::abs(initialPos.x - x) > player->movement){
                                    continue;
                                }

                                if(visited[y * 10 + x]){
                                    continue;
                                }
                                if(!c->walkable) continue;

                                glm::ivec2 cellPos = gridPosToWorld(&gs->worldGrid, x, y);
                                visited[y * 10 + x] = true;
                                w[y*10+x] = glm::abs(x - root.x) + glm::abs(y - root.y) + w[root.y * 10 + root.x];
                                neig[j++] = glm::ivec2(x,y);
                                String8 s = pushString8F(temp.arena, "%d", w[y*10+x]);
                                renderDrawText2D(&gs->f, s.str, cellPos, 1, ColorFromRGBA(0,0,0,255));
                                if(w[y*10+x] <= player->movement){
                                    renderDrawFilledRect(cellPos, glm::vec2(gs->worldGrid.cellSize), 0, ColorFromRGBA(204,102,0,255));
                                }
                                count++;
                            }
                        }
                    }
                    i++;
                }
            }
            if(!dragging && isMouseButtonJustPressed(MOUSE_BUTTON_LEFT) && (mouseGrid == player->pos)){
                initialPos = player->pos;
                dragging = true;
            } else if(dragging && isMouseButtonJustPressed(MOUSE_BUTTON_LEFT) &&
                    mouseGrid.x <= initialPos.x + player->movement && mouseGrid.x >= initialPos.x - player->movement &&
                    mouseGrid.y <= initialPos.y + player->movement && mouseGrid.y >= initialPos.y - player->movement ){
                if( mouseGrid.x >= 0 && mouseGrid.x <= gs->worldGrid.size.x &&
                    mouseGrid.y >= 0 && mouseGrid.y <= gs->worldGrid.size.y){

                    if(w[mouseGrid.y * 10 + mouseGrid.x] <= player->movement){
                        Cell* c = getGridCell(&gs->worldGrid, mouseGrid.x, mouseGrid.y);
                        if(c->walkable){
                            player->pos = mouseGrid;
                            dragging = false;
                            initialPos = player->pos;
                        }
                    }
                }
            }
            for(int i = 0; i < gs->maxUnits; i++){
                Color unitColor;
                Color playerColor = COLOR_WHITE;
                Color enemyColor  = COLOR_RED;
                Unit* unit = &gs->units[i];
                glm::vec2 unitPos = gridPosToWorld(&gs->worldGrid, unit->pos.x, unit->pos.y);
                if(unit->type == UNIT_PLAYER){
                    unitColor = playerColor;
                }else if(unit->type == UNIT_ENEMY){
                    unitColor = enemyColor;
                }
                renderDrawFilledRect(unitPos, glm::vec2(cellSize), 0, unitColor, LAYER_BG);
            }

            //debug center lines
            glm::vec2 p0x = glm::vec2(-gs->gameSize.x, 0);// gs->gameSize.y * 0.5f);
            glm::vec2 p1x = glm::vec2(gs->gameSize.x, 0);
            renderDrawLine(p0x, p1x, COLOR_WHITE);
            glm::vec2 p0y = glm::vec2(0, -gs->gameSize.y);// gs->gameSize.y * 0.5f);
            glm::vec2 p1y = glm::vec2(0, gs->gameSize.y);
            renderDrawLine(p0y, p1y, COLOR_WHITE);
        endMode2D();
    endTextureMode();
    endScene();
    

    //World drawing on texture to never lose the initial resolution
    glm::vec2 quadSize = gs->gameSize * scale;
    //glm::vec2 quadPos  = (getScreenSize() * 0.5f) - (quadSize * 0.5f); // top-left of quad
    //glm::vec2 quadPos  = glm::vec2((getScreenSize().x * 0.5f) - (quadSize.x * 0.5f), -quadSize.y * 0.5f); // top-left of quad

    String8 indexT = pushString8F(temp.arena, "grid: %d, %d", mouseGrid.x, mouseGrid.y);
    String8 mouseWorldT = pushString8F(temp.arena, "world: %f, %f", mouseWorld.x, mouseWorld.y);
    String8 mouseScreenT = pushString8F(temp.arena, "screen: %f, %f", mouseScreen.x, mouseScreen.y);

    glm::vec2 quadPos  = glm::vec2(0, 0); // top-left of quad
    Rect rect;
    rect.x = 0; rect.y = 0;
    rect.w = gs->gameSize.x; rect.h = gs->gameSize.y;
    glm::vec2 size = glm::vec2(rect.w, rect.h) * scale;
    glm::vec2 pos = (getScreenSize() * 0.5f) - (quadSize * 0.5f);// quadPos + glm::vec2(0, quadSize.y); // shift down by height to compensate for flip
    beginScene();
        clearColor(0.0f, 0.0f, 0.0f, 1.0f);
        renderDrawQuadPro2D(pos, {size.x, size.y},
                            0, rect, {0.0f,0.0f}, &gs->finalTexture.texture, COLOR_WHITE);
        renderDrawText2D(&gs->f, indexT.str ,{10,10}, 1, COLOR_WHITE);
        renderDrawText2D(&gs->f, mouseWorldT.str ,{10,30}, 1, COLOR_WHITE);
        renderDrawText2D(&gs->f, mouseScreenT.str ,{10,50}, 1, COLOR_WHITE);
    endScene();
    releaseTempArena(temp);

    //Ui Code -------------------------------------
    //drawHud(gs, gameArena);
}

GAME_API void gameStop(Arena* gameArena){
}