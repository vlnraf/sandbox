
//TODO: implement those in the new system
//Right now i copied them here to not lose the whole code
//but needs to be refactored with the new system
#if 0
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
#endif