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

    texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");

    bool ok = animations.LoadFromTSX(
        "Assets/Textures/slime.tsx",
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

    LOG("Enemy update, pos = (%f, %f)", position.getX(), position.getY());

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