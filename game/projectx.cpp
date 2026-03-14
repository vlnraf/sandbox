#include "projectx.hpp"
#include "datastructures.hpp"

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
    Vec2 pos;
    Vec2 size;
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
    Vec2 size;
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
    Vec2 pos;       // anchor point of the panel (screen space, y-up from bottom-left)
    Vec2 barPos;    // position of the title bar
    Vec2 barSize;   // size of the title bar
    Vec2 ctxPos;    // bottom-left of the content area (used for panel background rect)
    Vec2 ctxSize;   // accumulated content area size (grows as widgets are added)
    Vec2 cursorPos; // where the next widget will be placed
    float padding;
    float spacing;       // gap between widgets
};

struct UiState{
    Vec2 mousePos;
    uint32_t hover;
    uint32_t active;

    Font f;
    UiStyle style;
    Context c[3];
    uint16_t ctxCount;

    UiRenderCommandArray renderCommands;
};

bool pointInRect(Vec2 mousePos, Vec2 widgetPos, Vec2 widgetSize){
    return (mousePos.x > widgetPos.x && mousePos.x < widgetPos.x + widgetSize.x &&
            mousePos.y > widgetPos.y && mousePos.y < widgetPos.y + widgetSize.y);
}

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


void setStyle(UiState* ctx, const UiStyle* style){
    state->style = *style;
}

// Advance the cursor after placing a widget.
// With no layout: moves Y down, resets X to panel left (vertical stacking).
// With LAYOUT_HORIZONTAL: moves X right by cell width; wraps to next row after `items` cells.
// With LAYOUT_VERTICAL: moves Y down by cell height.
void advanceContext(Context* c, Vec2 size){
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
            c->ctxSize.x = maxF(c->ctxSize.x, c->layout->size.x);
            c->ctxSize.y = maxF(c->ctxSize.y, c->pos.y - c->cursorPos.y);
            c->ctxPos = {c->ctxPos.x, c->cursorPos.y - cellH};
        } else { // LAYOUT_VERTICAL
            float cellH = c->layout->size.y / (float)c->layout->items;
            c->cursorPos.y -= cellH;// + c->spacing;
            c->ctxSize.x = maxF(c->ctxSize.x, c->layout->size.x);
            c->ctxSize.y = maxF(c->ctxSize.y, c->pos.y - c->cursorPos.y);
        }
        c->ctxPos.y = minF(c->ctxPos.y, c->cursorPos.y);
        //c->ctxPos.y = glm::min(c->ctxPos.y, size.y);
    } else {
        // default vertical stacking: natural widget size
        c->cursorPos.y -= size.y;// + c->spacing;
        c->ctxSize.y = c->pos.y - c->barSize.y - c->cursorPos.y;
        c->ctxSize.x = maxF(c->ctxSize.x, size.x);
        c->ctxSize.x = maxF(c->ctxSize.x, c->barSize.x);
        c->cursorPos.x = c->pos.x;
        c->ctxPos.y = minF(c->ctxPos.y, c->cursorPos.y);
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
    Vec2 size;
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

    if(pointInRect(state->mousePos, pos, size)){
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

UiLayout makeLayout(UiLayoutType type, Vec2 size, uint32_t items){
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

void beginPanel(String8 title, Vec2 pos, Vec2 size){
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
    command.size.x  = maxF(c->barSize.x, c->ctxSize.x);
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
    Vec2 screenSize = getScreenSize();

    Vec2 panelSize = {250,250};
    Vec2 panelPos = {10,10};
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


Vec2 getMouseWorld(const GameState* gs, float scale){
    //mouse world calculations
    Vec2 mousePos = getMousePos();
    Vec2 n = getMousePos() / getScreenSize();
    Vec2 quadSize = gs->gameSize * scale;
    Vec2 quadPos  = (getScreenSize() * 0.5f) - (quadSize * 0.5f); // top-left of quad
    float quadLeft = quadPos.x;
    float quadRight = quadPos.x + quadSize.x;
    float quadTop = quadPos.y + quadSize.y;
    float quadBot = quadPos.y;
    Vec2 botLeft = {quadBot, quadLeft};
    Vec2 topRight = {quadTop, quadRight};
    mousePos = Vec2(getMousePos().x - quadLeft, getMousePos().y - quadBot);
    n = mousePos / quadSize;
    Vec2 worldPos = (n * gs->gameSize) - (gs->gameSize * 0.5f);

    return worldPos;
}

int ij(int i, int j, int width){
    return j * width + i;
}

Cell* getGridCell(const WorldGrid* grid, int i, int j){
    return &grid->cell[(grid->size.x * j) + i];
}

Unit* getUnitAtCell(const GameState* gs, int x, int y){
    for(int i = 0; i < gs->maxUnits; i++){
        if(gs->units[i].gridPos == (IVec2){x,y}) return &gs->units[i];
    }
    return NULL;
}

Vec2 gridPosToWorld(const WorldGrid* grid, int i, int j){
    //Vec2 center = Vec2(i + 0.5f , j + 0.5f);
    Vec2 center = Vec2(i, j);
    return Vec2(center.x * grid->cellSize, center.y * grid->cellSize) - (Vec2(grid->size) * grid->cellSize * 0.5f);
}

IVec2 worldPosToGrid(const WorldGrid* grid, Vec2 pos){
    Vec2 _fp = (pos + Vec2(grid->size) * grid->cellSize * 0.5f) / grid->cellSize;
    IVec2 gridPos = IVec2((int)floorF(_fp.x), (int)floorF(_fp.y));
    return gridPos;
}

bool isInWorldBoundary(const WorldGrid* grid, IVec2 point){
    return point.x >= 0 && point.x < grid->size.x && point.y >= 0 && point.y < grid->size.y;
}


/*
NOTE: if constructing the grid and zeroing memory each frame is too costly
      just construct it once and use a generation counter to check
      if the value in the cell is old or new.
      Later when we also need the path just return a structure instead of the array
      of cells with their weight

*/
int* bfsSearch(Arena* arena, WorldGrid* grid, int x, int y, int depth){
    IVec2* neig = arenaAllocArrayZero(arena, IVec2, grid->size.x * grid->size.y);
    bool* visited = arenaAllocArrayZero(arena, bool, grid->size.x * grid->size.y);
    int* w = arenaAllocArray(arena, int, grid->size.x * grid->size.y);
    int span = grid->size.x;
    for(int k = 0; k < grid->size.x * grid->size.y; k++) w[k] = -1;
    w[y * span + x] = 0;
    visited[y * span + x] = true;
    IVec2 initialPos = IVec2(x,y);
    neig[0] = initialPos;
    int i = 0;
    int j = 1;
    while(i != j){
        ASSERT((i <= j), "Overflow, in bfs queue");
        IVec2 root = neig[i];
        for(int yo = root.y -1; yo <= root.y + 1; yo++){
            for(int xo = root.x -1; xo <= root.x + 1; xo++){
                if( xo >= 0 && xo < grid->size.x &&
                    yo >= 0 && yo < grid->size.y){
                    
                    Cell* c = getGridCell(grid, xo, yo);
                    //No diagonal neighborhoods
                    if(absF(root.x - xo) + absF(root.y - yo) > 1){
                        continue;
                    }

                    //max depth
                    if(absF(initialPos.y - yo) + absF(initialPos.x - xo) > depth){
                        continue;
                    }

                    if(visited[yo * span + xo]){
                        continue;
                    }
                    //if(!c->walkable) continue;

                    Vec2 cellPos = gridPosToWorld(grid, xo, yo);
                    visited[yo * span + xo] = true;
                    w[yo*span+xo] = absF(xo - root.x) + absF(yo - root.y) + w[root.y * span + root.x];
                    neig[j++] = IVec2(xo,yo);
                }
            }
        }
        i++;
    }
    return w;
}

float lerp(float x, float y, float a){
    float result = 0;
    //newPos[gs->turnCount] = pos1 + (pos2 - pos1) * (a * t[gs->turnCount]);
    result = x * (1.0f - a) + y * a;
    return result;
}


//Units----------------------------------------------------------------------------------

void unitMovement(GameState* gs, Unit* u, WorldGrid* world, IVec2 dst, float dt){
    Cell* curCell   = getGridCell(world, u->gridPos.x, u->gridPos.y);
    Cell* dstCell = getGridCell(world, dst.x, dst.y);
    curCell->walkable = true;
    dstCell->walkable = false;
    u->destPos = dst;
}

bool moveUnit(GameState* gs, Unit* u, WorldGrid* world, float dt){
    bool result = false;
    Vec2 pos1 = gridPosToWorld(world, u->gridPos.x, u->gridPos.y);
    Vec2 pos2 = gridPosToWorld(world, u->destPos.x, u->destPos.y);
    if(u->a < 1.0f){
        float speed = 2.0f;
        u->a += speed * dt;
        u->worldPos = Vec2(lerp(pos1.x, pos2.x, u->a), lerp(pos1.y, pos2.y, u->a));
        result = false;
    }else{
        u->gridPos = u->destPos;
        u->a = 0;
        result = true;
    }
    return result;
}


GAME_API const char* applicationSetup(){
    return "Test";
}

GAME_API uint32_t gameStart(Arena* gameArena){
    if(gameArena->index > 0){
        return sizeof(GameState);
    }
    GameState* gs = arenaAllocStruct(gameArena, GameState);
    gs->arena = gameArena;
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
    IVec2 gridSize = {25, 15};
    gs->worldGrid.size = gridSize;
    gs->worldGrid.cell = arenaAllocArray(gs->arena, Cell, gridSize.x * gridSize.y);
    gs->worldGrid.cellSize = 16;
    for(int j = 0; j < gs->worldGrid.size.y; j++){
        for(int i = 0; i < gs->worldGrid.size.x; i++){
            //TODO initialize cells
            Cell* c     = getGridCell(&gs->worldGrid, i, j);
            c->walkable = true;
            c->color    = ColorFromRGBA(51, 128, 51, 255);
        }
    }

    // Units
    gs->maxUnits = 5;
    gs->units = arenaAllocArray(gs->arena, Unit, gs->maxUnits);
    for(int i = 1; i < gs->maxUnits; i++){
        gs->units[i].type            = UNIT_ENEMY;
        gs->units[i].status          = STATUS_ALIVE;
        Vec2 pos                     = Vec2(rand() % gridSize.x, rand() % gridSize.y);
        gs->units[i].gridPos         = pos;
        gs->units[i].destPos         = pos;
        gs->units[i].actionPoints    = 1;
        gs->units[i].movement        = 2;
        gs->units[i].dmg             = 1;
        gs->units[i].hp              = 2;
        gs->units[i].worldPos        = gridPosToWorld(&gs->worldGrid, gs->units[i].gridPos.x, gs->units[i].gridPos.y);
        gs->units[i].color           = COLOR_RED;
        gs->units[i].actionType      = ACTION_NONE;
        gs->units[i].attackRange     = 2;
        gs->units[i].dName           = String8Lit("E");

        Cell* cell      = getGridCell(&gs->worldGrid, pos.x, pos.y);
        cell->walkable  = false;
    }
    Vec2 pos = (Vec2){0,0};
    gs->units[0].type           = UNIT_PLAYER;
    gs->units[0].status         = STATUS_ALIVE;
    gs->units[0].gridPos        = pos;
    gs->units[0].destPos        = pos;
    gs->units[0].actionPoints   = 1;
    gs->units[0].movement       = 3;
    gs->units[0].dmg            = 1;
    gs->units[0].hp             = 10;
    gs->units[0].worldPos       = gridPosToWorld(&gs->worldGrid, gs->units[0].gridPos.x, gs->units[0].gridPos.y);
    gs->units[0].color          = COLOR_WHITE;
    gs->units[0].selected       = false;
    gs->units[0].actionType     = ACTION_NONE;
    gs->units[0].attackRange    = 5;
    gs->units[0].dName          = String8Lit("P");

    Cell* cell      = getGridCell(&gs->worldGrid, pos.x, pos.y);
    cell->walkable  = false;

    gs->gameTextures[PLAYER_TEXTURE] = loadWhiteTexture();

    gs->turn        = 1;
    gs->turnCount   = 0;
    gs->turnUnits   = 0;
    gs->turnQueue   = arenaAllocArray(gs->arena, Unit*, gs->maxUnits);
    return sizeof(GameState);
}

GAME_API void gameRender(Arena* gameArena, float dt){}

GAME_API void gameUpdate(Arena* gameArena, float dt){
    GameState* gs = (GameState*)gameArena->memory;
    //---------------------------------------
    //trick to hot reload and prototype faster
    //TODO: remove
    //state = gs->uiState;
    //---------------------------------------
    if(gs->turnCount == gs->turnUnits){
        gs->turnCount = 0;
        gs->turnUnits = 0;
        for(int i = 0; i < gs->maxUnits; i++){
            if(gs->units[i].status == STATUS_DEATH) continue;
            gs->turnQueue[gs->turnUnits] = &gs->units[i];
            gs->units[i].actionPoints = 1;
            gs->turnUnits++;
        }
        gs->turn++;
    }
    //if(gs->units[gs->turnCount].status == STATUS_DEATH) gs->turnCount++; //Death units don't move

    //for(int i = 0; i < gs->maxUnits; i++){
    //    if(gs->units[i].type == UNIT_PLAYER){
    //        printf("P ");
    //    }else if(gs->units[i].type == UNIT_ENEMY){
    //        printf("E ");
    //    }
    //}
    //printf("\n");
    //for(int i = 0; i < gs->turnCount; i++){
    //    printf("  ");
    //}
    //printf("^");
    //printf("\n");

    Unit* unit = gs->turnQueue[gs->turnCount];

    TempArena temp = getTempArena(gs->arena);
    float scale = 2.0f;
    Vec2 mouseScreen = getMousePos();
    Vec2 mouseWorld = getMouseWorld(gs, scale);
    IVec2 mouseGrid = worldPosToGrid(&gs->worldGrid, mouseWorld);

    //render the game into the texture world
    IVec2 gridSize = gs->worldGrid.size;
    float cellSize = gs->worldGrid.cellSize;

    int* w = bfsSearch(temp.arena, &gs->worldGrid, unit->gridPos.x, unit->gridPos.y, unit->movement);
    int span = gs->worldGrid.size.x;

    //TODO: make a dispatcher???
    if(unit->type == UNIT_PLAYER){
        Vec2 cellPos = gridPosToWorld(&gs->worldGrid, unit->gridPos.x , unit->gridPos.y);
        Unit* enemySelected;
        if(unit->selected){
            if(isMouseButtonJustPressed(MOUSE_BUTTON_RIGHT)){
                unit->selected = false;
            }
        }
        if(!unit->selected && isMouseButtonJustPressed(MOUSE_BUTTON_LEFT) && (mouseGrid == unit->gridPos)){ //player action
            unit->selected = true;
        } else if(unit->selected && isMouseButtonJustPressed(MOUSE_BUTTON_LEFT) && gs->turnCount == 0){ 
            enemySelected = getUnitAtCell(gs, mouseGrid.x, mouseGrid.y);
            if(enemySelected)
                unit->actionType = ACTION_ATTACK;

            if(isInWorldBoundary(&gs->worldGrid, mouseGrid)){
                if(mouseGrid.x <= unit->gridPos.x + unit->movement && mouseGrid.x >= unit->gridPos.x - unit->movement
                && mouseGrid.y <= unit->gridPos.y + unit->movement && mouseGrid.y >= unit->gridPos.y - unit->movement ){
                    int span = gs->worldGrid.size.x;
                    if(w[mouseGrid.y * span + mouseGrid.x] <= unit->movement && w[mouseGrid.y * span + mouseGrid.x] > 0){
                        Cell* c = getGridCell(&gs->worldGrid, mouseGrid.x, mouseGrid.y);
                        if(c->walkable){
                            unit->actionType = ACTION_MOVE;
                            unit->selected = false;
                            unitMovement(gs, unit, &gs->worldGrid, mouseGrid, dt);
                        }
                    }
                }
            }
        }
        if(unit->actionType == ACTION_ATTACK){
            enemySelected->hp -= unit->dmg;
            unit->selected = false;
            unit->actionPoints--;
            LOGINFO("%d", enemySelected->hp);
        }else if(unit->actionType == ACTION_MOVE){
            if(moveUnit(gs, unit, &gs->worldGrid, dt)){
                unit->actionPoints--;
            }
        }
        if(unit->actionPoints == 0){
            unit->actionType   = ACTION_NONE;
            unit->selected = false;
            gs->turnCount++;
        }
    }else if(unit->type == UNIT_ENEMY){
        int span = gs->worldGrid.size.x;
        int bestCost = 0;
        Vec2 bestPos;
        IVec2 reachable[100];
        int reachableCount = 0;
        for(int j = unit->gridPos.y - unit->movement; j <= unit->gridPos.y + unit->movement; j++){
            for(int i = unit->gridPos.x - unit->movement; i <= unit->gridPos.x + unit->movement; i++){
                if(!isInWorldBoundary(&gs->worldGrid, (Vec2){i,j})) continue;
                int index = ij(i,j,span);
                int cost = w[index];
                if(cost > bestCost && cost <= unit->movement){
                    IVec2 pos = {i, j};
                    reachable[reachableCount++] = pos;
                }
            }
        }

        Unit* target;
        if(unit->hp <= 0){
            unit->actionPoints = 0;
            unit->actionType = ACTION_NONE;
        }else{
            for(int i = 0; i < reachableCount; i++){
                target = getUnitAtCell(gs, reachable[i].x, reachable[i].y);
                if(target && target->type == UNIT_PLAYER){
                    unit->actionType = ACTION_ATTACK;
                    break;
                }
            }
            if(unit->actionType == ACTION_NONE){
                unit->actionType = ACTION_MOVE;
                unit->destPos = reachable[rand() % reachableCount];
                unitMovement(gs, unit, &gs->worldGrid, unit->destPos, dt);
            }

            if(unit->actionType == ACTION_ATTACK){
                target->hp -= unit->dmg;
                unit->actionPoints--;
                LOGINFO("%d", target->hp);
            }else if(unit->actionType == ACTION_MOVE){
                if(moveUnit(gs, unit, &gs->worldGrid, dt)){
                    unit->actionPoints--;
                }
            }
        }
        if(unit->actionPoints == 0){
            unit->actionType   = ACTION_NONE;
            gs->turnCount++;
        }
    }

    //death system
    for(int i = 0; i < gs->maxUnits; i++){
        if(gs->units[i].hp <= 0){
            gs->units[i].status = STATUS_DEATH;
        }
    }

    beginScene(NO_DEPTH);
    clearColor(0.0f,0.0f,0.0f,1);
    beginTextureMode(&gs->finalTexture);
        beginMode2D(gs->mainCamera);
            //rendering grid
            renderDrawFilledRect(-(gs->gameSize / 2.0f), gs->gameSize, 0, ColorFromRGBA(0,0,128,255), LAYER_BG);
            for(int j = 0; j < gridSize.y; j++){
                for(int i = 0; i < gridSize.x; i++){
                    Cell* c = getGridCell(&gs->worldGrid, i, j);
                    Vec2 cellPos = gridPosToWorld(&gs->worldGrid, i , j);
                    Color borderColor = ColorFromRGBA( 0,   0,  0, 255);
                    if(pointInRect(mouseWorld, cellPos, {16,16})){
                        renderDrawFilledRectPro(cellPos, Vec2(cellSize), 0, {0.0, 0.0}, COLOR_BLUE, LAYER_BG);
                    }else{
                    renderDrawFilledRectPro(cellPos, Vec2(cellSize), 0, {0.0, 0.0}, c->color, LAYER_BG);
                    renderDrawRect(cellPos, Vec2(cellSize), borderColor, LAYER_BG);
                    }
                }
            }

            //rendering bfs cells
            //rendering every unit for debug later render only for player units
            for(int j = unit->gridPos.y - unit->movement; j <= unit->gridPos.y + unit->movement; j++){
                for(int i = unit->gridPos.x - unit->movement; i <= unit->gridPos.x + unit->movement; i++){
                    if(!isInWorldBoundary(&gs->worldGrid, (Vec2){i,j})) continue;
                    int index = ij(i,j, gs->worldGrid.size.x);
                    if(w[index] >= 0 && w[index] <= unit->movement){
                        Vec2 cellPos = gridPosToWorld(&gs->worldGrid, i, j);
                        String8 s = pushString8F(temp.arena, "%d", w[index]);
                        renderDrawText2D(&gs->f, s.str, cellPos, 1, ColorFromRGBA(0,0,0,255));
                        renderDrawFilledRect(cellPos, Vec2(gs->worldGrid.cellSize), 0, ColorFromRGBA(204,102,0,255));
                    }
                }
            }

            //rendering all the units
            for(int i = 0; i < gs->maxUnits; i++){
                Unit* u = &gs->units[i];
                renderDrawFilledRect(u->worldPos, Vec2(cellSize), 0, u->color, LAYER_BG);
            }

            //debug center lines
            Vec2 p0x = Vec2(-gs->gameSize.x, 0);// gs->gameSize.y * 0.5f);
            Vec2 p1x = Vec2(gs->gameSize.x, 0);
            renderDrawLine(p0x, p1x, COLOR_WHITE);
            Vec2 p0y = Vec2(0, -gs->gameSize.y);// gs->gameSize.y * 0.5f);
            Vec2 p1y = Vec2(0, gs->gameSize.y);
            renderDrawLine(p0y, p1y, COLOR_WHITE);


        endMode2D();

        //Queue simple render
        //Layout -------------------
        Vec2 textureSize = (Vec2){(float)gs->finalTexture.texture.width, (float)gs->finalTexture.texture.height};
        int spacingX = 1;
        int spacingY = 5;
        int cardNum = gs->turnUnits;
        Vec2 cardSize = (Vec2){30, 40};
        Vec2 cardPos = (Vec2){(textureSize.x * 0.5f) - (((cardSize.x + spacingX) * cardNum) * 0.5f), textureSize.y - spacingY};
        for(int i = 0; i < cardNum; i++){
            renderDrawFilledRectPro(cardPos, cardSize, 0, {0,1}, COLOR_WHITE);
            Unit* u = &gs->units[i];

            float tScale = 1.0f; //text scale
            float textHeight = calculateTextHeight(&gs->f, u->dName.str, tScale);
            float textWidth  = calculateTextWidth(&gs->f, u->dName.str, tScale);
            Vec2 textPos = (Vec2){cardPos.x + cardSize.x * 0.5f, cardPos.y - cardSize.y * 0.5f};
            textPos.x = textPos.x - textWidth * 0.5f;
            textPos.y = textPos.y - textHeight * 0.5f;
            renderDrawText2D(&gs->f, u->dName.str, textPos, tScale, COLOR_BLACK);
            if(i == gs->turnCount){
                Vec2 queueCurPos = cardPos;
                Vec2 queueCurSize = (Vec2){10,10};
                queueCurPos.x += cardSize.x * 0.5f - queueCurSize.x * 0.5f;
                queueCurPos.y -= cardSize.y + spacingY;
                renderDrawFilledRectPro(queueCurPos, queueCurSize, 0, {0,1}, COLOR_YELLOW);
            }

            //advance card cursor
            cardPos.x = cardPos.x + cardSize.x + spacingX;
        }
    endTextureMode();
    endScene();
    

    //World drawing on texture to never lose the initial resolution
    Vec2 quadSize = gs->gameSize * scale;
    //Vec2 quadPos  = (getScreenSize() * 0.5f) - (quadSize * 0.5f); // top-left of quad
    //Vec2 quadPos  = Vec2((getScreenSize().x * 0.5f) - (quadSize.x * 0.5f), -quadSize.y * 0.5f); // top-left of quad

    String8 indexT = pushString8F(temp.arena, "grid: %d, %d", mouseGrid.x, mouseGrid.y);
    String8 mouseWorldT = pushString8F(temp.arena, "world: %f, %f", mouseWorld.x, mouseWorld.y);
    String8 mouseScreenT = pushString8F(temp.arena, "screen: %f, %f", mouseScreen.x, mouseScreen.y);

    Vec2 quadPos  = Vec2(0, 0); // top-left of quad
    Rect rect;
    rect.x = 0; rect.y = 0;
    rect.w = gs->gameSize.x; rect.h = gs->gameSize.y;
    Vec2 size = Vec2(rect.w, rect.h) * scale;
    Vec2 pos = (getScreenSize() * 0.5f) - (quadSize * 0.5f);// quadPos + Vec2(0, quadSize.y); // shift down by height to compensate for flip
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