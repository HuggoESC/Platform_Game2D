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
    
}

Enemy::~Enemy()
{
}

bool Enemy::Update(float dt)
{
    // 1) Actualizar animación
    animations.Update(dt);

    // 2) Sincronizar posición visual con el cuerpo Box2D
    int px, py;
    pbody->GetPosition(px, py);
    position.setX((float)px);
    position.setY((float)py);

    // 3) Movimiento simple DENTRO de Box2D (100% estable)
    float velX = direction * speed;         // velocidad horizontal
    Engine::GetInstance().physics->SetXVelocity(pbody, velX);

    // 4) Dibujar en pantalla
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
	direction *= -1;
}