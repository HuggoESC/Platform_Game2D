#include "UIButton.h"
#include "Engine.h"
#include "Render.h"
#include "Input.h"
#include "Scene.h"

UIButton::UIButton(int id, const std::string& text, const SDL_Rect& rect, Scene* callback)
    : UIElement(id, UIElementType::BUTTON, rect, callback), text(text)
{
}

void UIButton::Update(float dt)
{
    int mx, my;
    Engine::GetInstance().input->GetMousePosition(mx, my);

    hovered = SDL_PointInRect(&SDL_Point{ mx, my }, &rect);

    if (hovered && Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT))
    {
        callback->OnUIMouseClickEvent(this);
    }
}

void UIButton::Draw()
{
    SDL_Color color = hovered ? SDL_Color{ 200, 200, 50, 255 } : SDL_Color{ 255, 255, 255, 255 };

    Engine::GetInstance().render->DrawRectangle(rect, color, true);

    Engine::GetInstance().render->DrawText(text.c_str(),
        rect.x + 10,
        rect.y + 5,
        rect.w - 20,
        rect.h - 10,
        { 0,0,0,255 });
}