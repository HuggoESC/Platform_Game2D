#include "Enemy.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"
#include "Physics.h"
#include "Input.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Map.h"
#include "Audio.h"


Enemy::Enemy(int x, int y)
{
    type = EntityType::ENEMY;

    texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
    animations.LoadFromTSX("Assets/Textures/slime.tsx",
        { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
    animations.SetCurrent("walkR");

    // No usamos x,y todavía ? lo colocaremos con SetPosition()
    pbody = Engine::GetInstance().physics->CreateCircle(x, y, 14, DYNAMIC);
    pbody->listener = this;
    b2Body_SetFixedRotation(pbody->body, true);
}

Enemy::~Enemy()
{
}

bool Enemy::Update(float dt)
{
    animations.Update(dt);

    int px, py;
    pbody->GetPosition(px, py);
    position.setX((float)px);
    position.setY((float)py);

    if (sensorFront && sensorBack)
    {
        const int SENSOR_HEIGHT_OFFSET = 14;

        sensorFront->SetPosition(px + sensorOffset * direction, py - SENSOR_HEIGHT_OFFSET);
        sensorBack->SetPosition(px - sensorOffset * direction, py - SENSOR_HEIGHT_OFFSET);
    }

    // Movimiento estable, sin rebotes automáticos
    float velX = direction * speed;
    Engine::GetInstance().physics->SetXVelocity(pbody, velX);

    SDL_Rect frame = animations.GetCurrentFrame();
    Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);
    return true;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physA == sensorFront && physB->ctype == ColliderType::WALL)
        direction = -1;

    if (physA == sensorBack && physB->ctype == ColliderType::WALL)
        direction = 1;
}