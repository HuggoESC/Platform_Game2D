#pragma once

#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

class PhysBody;

// Collectible Gem (from Tiled: name/class "Gem")
class Gem : public Entity
{
public:
    Gem(int x, int y);
    virtual ~Gem() = default;

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    bool Destroy();

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
    AnimationSet anims;

    int texW = 32;
    int texH = 32;
};

