#pragma once

#include "UIElement.h"
#include <string>

class UIButton : public UIElement
{
public:
    UIButton(int id, const std::string& text, const SDL_Rect& rect, Scene* callback);

    void Update(float dt) override;
    void Draw() override;

public:
    std::string text;
    bool hovered = false;
};

