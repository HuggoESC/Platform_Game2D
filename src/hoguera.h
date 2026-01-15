#pragma once

#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

class PhysBody;

class hoguera : public Entity
{
public:
    hoguera(int x, int y);

	bool CleanUp() override;

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
    AnimationSet anims;

    bool activated = false; 

    int texW = 32;
    int texH = 32;
};