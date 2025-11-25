#include "Enemy.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"

Enemy::Enemy(int x, int y)
{
    type = EntityType::ENEMY;

    position.setX((float)x);
    position.setY((float)y);

    texture = Engine::GetInstance().textures->Load("textures/slime.png");

    bool ok = animations.LoadFromTSX(
        "textures/slime.tsx",
        {
            {28, "idle"}  // anim default
        }
    );

    if (!ok)
        LOG("ERROR loading slime.tsx animations");

    animations.SetCurrent("idle");
}

Enemy::~Enemy()
{
}

bool Enemy::Update(float dt)
{
    // update animación
    animations.Update(dt);

    // obtener frame actual
    SDL_Rect frame = animations.GetCurrentFrame();

    // dibujar
    Engine::GetInstance().render->DrawTexture(
        texture,
        (int)position.getX(),
        (int)position.getY(),
        &frame
    );

    return true;
}