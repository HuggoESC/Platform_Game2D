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
    sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x + 20, y, 6, 6, STATIC);
    sensorFront->listener = this;
    sensorFront->ctype = ColliderType::SENSOR;

    sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x - 20, y, 6, 6, STATIC);
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

    b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
    vel.x = direction * speed;
    Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
  /*  sensorFront->SetPosition(px + 16 * direction, py);
    sensorBack->SetPosition(px - 16 * direction, py);*/

    SDL_Rect frame = animations.GetCurrentFrame();
    Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);

    return true;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // Consideramos sólido todo lo que no sea jugador, sensor o desconocido
    bool bloqueSolido =
        physB->ctype != ColliderType::PLAYER &&
        physB->ctype != ColliderType::SENSOR &&
        physB->ctype != ColliderType::UNKNOWN;

    // Solo nos interesa cuándo el QUE choca es el cuerpo del slime
    if (physA == pbody && bloqueSolido)
    {
        direction *= -1;   // invertimos dirección
        LOG("SLIME: cambio de dirección");
    }
}