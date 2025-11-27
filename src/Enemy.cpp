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

    // Posición inicial
    position.setX((float)x);
    position.setY((float)y);

    // Textura y animaciones
    texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
    animations.LoadFromTSX("Assets/Textures/slime.tsx",
        { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
    animations.SetCurrent("walkR"); // de momento hacia la derecha

    //  Cuerpo físico principal 
    pbody = Engine::GetInstance().physics->CreateCircle(x, y, 14, DYNAMIC);
    pbody->listener = this;
    b2Body_SetFixedRotation(pbody->body, true);

    // Sensores: pequeños y algo elevados para no tocar el suelo 
    int sensorRadius = 4;
    int sensorY = y - 10;                    // 10 píxeles por encima del centro
    sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(
        x + (int)sensorOffset, y - 10, 6, 6, STATIC); // pequeño y no participa en físicas
    sensorFront->listener = this;
    sensorFront->ctype = ColliderType::SENSOR;

    sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(
        x - (int)sensorOffset, y - 10, 6, 6, STATIC);
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