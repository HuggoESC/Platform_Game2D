#include "UIManager.h"
#include "UIButton.h"
#include "Engine.h"
#include "Render.h"

UIManager::UIManager()
{
    name = "uimanager";
}

UIManager::~UIManager()
{
}

bool UIManager::Awake() { return true; }
bool UIManager::Start() { return true; }

bool UIManager::Update(float dt)
{
    for (auto& e : elements)
    {
        e->Update(dt);
        e->Draw();
    }

    return true;
}

bool UIManager::CleanUp()
{
    elements.clear();
    return true;
}

std::shared_ptr<UIElement> UIManager::CreateElement(UIElementType type,
    int id,
    const std::string& text,
    const SDL_Rect& rect,
    Scene* callback)
{
    std::shared_ptr<UIElement> elem = nullptr;

    if (type == UIElementType::BUTTON)
        elem = std::make_shared<UIButton>(id, text, rect, callback);

    if (elem)
        elements.push_back(elem);

    return elem;
}

void UIManager::RemoveAll()
{
    elements.clear();
}