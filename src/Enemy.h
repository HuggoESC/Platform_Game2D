#pragma once

#include "Entity.h"
#include "Animation.h"
#include "SDL3/SDL.h"

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

    bool Update(float dt) override;

private:
    AnimationSet animations;
    SDL_Texture* texture = nullptr;
};