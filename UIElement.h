#pragma once

#include <SDL3/SDL.h>
#include <string>

class Scene;

enum class UIElementType
{
    BUTTON,
    LABEL
};

class UIElement
{
public:
    UIElement(int id, UIElementType type, const SDL_Rect& rect, Scene* callback)
        : id(id), type(type), rect(rect), callback(callback)
    {
    }

    virtual ~UIElement() {}

    virtual void Update(float dt) {}
    virtual void Draw() {}

public:
    int id;                 // Identificador del elemento
    UIElementType type;     // Tipo (botón, label)
    SDL_Rect rect;          // Posición en pantalla
    Scene* callback;        // A quién avisar cuando se haga clic
};

