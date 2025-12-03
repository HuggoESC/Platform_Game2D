#pragma once

#include "Entity.h"
#include "Vector2D.h"
#include <SDL3/SDL.h>

// Forward declaration para poder usar PhysBody*
class PhysBody;

class hoguera : public Entity
{
public:
    hoguera(int x, int y);
   /* ~hoguera() override = default;*/

    bool Update(float dt) override;
    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
    Vector2D position;
};