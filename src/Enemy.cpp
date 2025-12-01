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

    // Cuerpo fisico
    pbody = Engine::GetInstance().physics->CreateCircle(x, y, 14, DYNAMIC);
    pbody->listener = this;
    b2Body_SetFixedRotation(pbody->body, true);
    pbody->ctype = ColliderType::ENEMY;

	// Sensores laterales
    sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x + 20, y, 6, 6, DYNAMIC);
    sensorFront->listener = this;
    sensorFront->ctype = ColliderType::SENSOR;

    sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x - 20, y, 6, 6, DYNAMIC);
    sensorBack->listener = this;
    sensorBack->ctype = ColliderType::SENSOR;

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

    // Reposicionar sensores
    sensorFront->SetPosition(px + sensorOffset * direction, py - 10);
    sensorBack->SetPosition(px - sensorOffset * direction, py - 10);

    // Movimiento constante
    Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);

    // Dibujar
    SDL_Rect frame = animations.GetCurrentFrame();
    Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);

    return true;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // Sensor delantero toca algo sólido ? gira a la izquierda
    if (physA == sensorFront &&
        (physB->ctype == ColliderType::WALL || physB->ctype == ColliderType::PLATFORM))
        direction = -1;

    // Sensor trasero toca algo sólido ? gira a la derecha
    if (physA == sensorBack &&
        (physB->ctype == ColliderType::WALL || physB->ctype == ColliderType::PLATFORM))
        direction = 1;
}