#pragma once

#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

class PhysBody;

class LifeUP : public Entity
{
public:
    LifeUP(int x, int y);
    virtual ~LifeUP() = default;

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // Para poder destruirlo desde Player (como el Item)
    bool Destroy();

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
    AnimationSet anims;

    int texW = 32;
    int texH = 32;
};

