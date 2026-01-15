#include "LifeUP.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "EntityManager.h"

LifeUP::LifeUP(int x, int y) : Entity(EntityType::LIFEUP)
{
    name = "LifeUP";
    position.setX((float)x);
    position.setY((float)y);
}

bool LifeUP::Awake()
{
    return true;
}


bool LifeUP::Start()
{
    texture = Engine::GetInstance().textures->Load("Assets/Textures/LifeUP.png");
    Engine::GetInstance().textures->GetSize(texture, texW, texH);

    std::unordered_map<int, std::string> aliases = { {0, "idle"} };
    anims.LoadFromTSX("Assets/Textures/LifeUP.tsx", aliases);
    anims.SetCurrent("idle");

    // El tamaño real que queremos para colisión y draw es el del frame (normalmente 32x32)
    SDL_Rect frame = anims.GetCurrentFrame();
    int fw = frame.w;
    int fh = frame.h;

    // En Tiled el objeto te llega como x,y (arriba-izq). Convertimos a centro:
    int cx = (int)position.getX() + fw / 2;
    int cy = (int)position.getY() + fh / 2;

    // Sensor rectangular (NO bloquea) y STATIC (no tiene por qué moverse)
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(cx, cy, fw, fh, bodyType::STATIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::LIFEUP;

    return true;
}

bool LifeUP::Update(float dt)
{
    if (!active) return true;

    anims.Update(dt);

    SDL_Rect frame = anims.GetCurrentFrame();

    int x, y;
    if (pbody)
    {
        pbody->GetPosition(x, y); // centro del body
    }
    else
    {
        x = (int)position.getX() + frame.w / 2;
        y = (int)position.getY() + frame.h / 2;
    }

    Engine::GetInstance().render->DrawTexture(
        texture,
        x - frame.w / 2,
        y - frame.h / 2,
        &frame
    );

    return true;
}

void LifeUP::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // La lógica principal se hará en Player::OnCollision (sumar vida).
    if (physB->ctype == ColliderType::PLAYER)
    {
        LOG("LifeUP tocado por el jugador");
    }
}

bool LifeUP::CleanUp()
{
    if (texture)
    {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    if (pbody)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }

    return true;
}

bool LifeUP::Destroy()
{
    LOG("Destroying LifeUP");
    active = false;
    Engine::GetInstance().entityManager->DestroyEntity(shared_from_this());
    return true;
}