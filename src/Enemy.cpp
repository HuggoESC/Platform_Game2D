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

    // Posición inicial lógica
    position.setX((float)x);
    position.setY((float)y);

    // Textura y animaciones
    texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
    animations.LoadFromTSX("Assets/Textures/slime.tsx",
        { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
    animations.SetCurrent("walkR");

	// cuerpo físico
    pbody = Engine::GetInstance().physics->CreateCircle(x, y, 14, DYNAMIC);
    pbody->listener = this;
    b2Body_SetFixedRotation(pbody->body, true);
    pbody->ctype = ColliderType::ENEMY;

	// sensores delantero y trasero
    sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x + 20, y, 6, 6, DYNAMIC);
    sensorFront->listener = this;
    sensorFront->ctype = ColliderType::SENSOR;

    sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x - 20, y, 6, 6, DYNAMIC);
    sensorBack->listener = this;
    sensorBack->ctype = ColliderType::SENSOR;
}

Enemy::~Enemy()
{
}

bool Enemy::Update(float dt)
{
    animations.Update(dt);

    int px, py;
    pbody->GetPosition(px, py);

    Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);

    sensorFront->SetPosition(px + 16 * direction, py);
    sensorBack->SetPosition(px - 16 * direction, py);

    SDL_Rect frame = animations.GetCurrentFrame();
    Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);

    return true;
}

void Enemy::OnCollision(PhysBody* a, PhysBody* b)
{
    bool isWall = (b->ctype == ColliderType::WALL);
    bool isPlatform = (b->ctype == ColliderType::PLATFORM);

    // gira si el sensor frontal toca muro o plataforma (azul o verde)
    if (a == sensorFront && (isWall || isPlatform))
        direction = -1;

    // gira si el sensor trasero toca muro o plataforma
    if (a == sensorBack && (isWall || isPlatform))
        direction = 1;
}