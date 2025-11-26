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
    sensorFront = Engine::GetInstance().physics->CreateCircle(x + (int)sensorOffset, sensorY, sensorRadius, DYNAMIC);
    sensorFront->listener = this;

    sensorBack = Engine::GetInstance().physics->CreateCircle(x - (int)sensorOffset, sensorY, sensorRadius, DYNAMIC);
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
        sensorFront->SetPosition(px + (int)(sensorOffset * direction), py-10);
        sensorBack->SetPosition(px - (int)(sensorOffset * direction), py-10);
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
    // Solo nos interesan colisiones con el suelo/plataformas
    if (physB->ctype != ColliderType::PLATFORM && physB->ctype != ColliderType::TOPE)
        return;

    // Posición del slime (cuerpo principal)
    int ex, ey;
    pbody->GetPosition(ex, ey);

    // Posición del objeto con el que hemos chocado
    int ox, oy;
    physB->GetPosition(ox, oy);

    int diffX = ox - ex;
    int diffY = oy - ey;

    // Colisión lateral: la diferencia en X es mayor que en Y
    bool lateral = abs(diffX) > abs(diffY);

    if (!lateral)
        return; // probablemente suelo ? ignoramos

    // Si el que choca es el sensor frontal ? giramos hacia el lado contrario
    if (physA == sensorFront)
    {
        direction = -1;          // ir hacia la izquierda
        animations.SetCurrent("walkL");
    }
    // Si el que choca es el sensor trasero ? giramos hacia la derecha
    else if (physA == sensorBack)
    {
        direction = 1;           // ir hacia la derecha
        animations.SetCurrent("walkR");
    }
}