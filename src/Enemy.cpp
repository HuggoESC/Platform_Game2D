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
        animations.LoadFromTSX("Assets/Textures/slime.tsx", { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
        animations.SetCurrent("idle");

		// Cuerpo físico
        pbody = Engine::GetInstance().physics->CreateCircle(x, y, 14, DYNAMIC);
        pbody->listener = this;
        b2Body_SetFixedRotation(pbody->body, true);   // Para que no rote al colisionar

        // sensores
        sensorFront = Engine::GetInstance().physics->CreateCircle(x + sensorOffset, y, 6, DYNAMIC);
        sensorFront->listener = this;
        b2Body_SetFixedRotation(pbody->body, true);

        // sensores
        sensorFront = Engine::GetInstance().physics->CreateCircle(x + (int)sensorOffset, y, 6, DYNAMIC);
        sensorFront->listener = this;

        sensorBack = Engine::GetInstance().physics->CreateCircle(x - (int)sensorOffset, y, 6, DYNAMIC);
        sensorBack->listener = this;
    
}

Enemy::~Enemy()
{
}

bool Enemy::Update(float dt)
{
    // Actualizar animación
    animations.Update(dt);

    // Sincronizar posición visual con el cuerpo Box2D
    int px, py;
    pbody->GetPosition(px, py);
    position.setX((float)px);
    position.setY((float)py);

	if (sensorFront && sensorBack) // sinconizar sensores
    {
        sensorFront->SetPosition(px + sensorOffset * direction, py);
        sensorBack->SetPosition(px - sensorOffset * direction, py);
    }

    // Movimiento simple DENTRO de Box2D (100% estable)
    float velX = direction * speed;         // velocidad horizontal
    Engine::GetInstance().physics->SetXVelocity(pbody, velX);

    // Dibujar en pantalla
    SDL_Rect frame = animations.GetCurrentFrame();
    Engine::GetInstance().render->DrawTexture(
        texture,
        px - 16,         // centrado del sprite (32/2)
        py - 16,
        &frame
    );

    return true;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // Si el que colisiona es el sensor forward ? girar
    if (physA == sensorFront && physB->ctype == ColliderType::PLATFORM) 
    {
        direction = -1; // Girar a la izquierda
       
    }

    // Si colisiona el sensor trasero ? girar al otro lado
    if (physA == sensorBack && physB->ctype == ColliderType::PLATFORM) 
    {
        direction = 1; // Girar a la derecha
     
    }
}