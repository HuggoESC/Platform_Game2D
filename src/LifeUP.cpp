#include "LifeUP.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "EntityManager.h"

LifeUP::LifeUP(int x, int y)
    : Entity(EntityType::ITEM)    // usamos ITEM como tipo genérico de entidad
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
    // Cargar textura 
    texture = Engine::GetInstance().textures->Load("Assets/Textures/LifeUP.png");
    Engine::GetInstance().textures->GetSize(texture, texW, texH);

    // Cargar animación desde el TSX 
    std::unordered_map<int, std::string> aliases = { {0, "idle"} };
    anims.LoadFromTSX("Assets/Textures/LifeUP.tsx", aliases);
    anims.SetCurrent("idle");

    // Cuerpo físico (círculo)
    pbody = Engine::GetInstance().physics->CreateCircle(
        (int)position.getX(),
        (int)position.getY(),
        texW / 2,
        bodyType::DYNAMIC
    );

    pbody->listener = this;
    pbody->ctype = ColliderType::LIFEUP;

    return true;
}

bool LifeUP::Update(float dt)
{
    if (!active) return true;

    // dt a segundos
    float dtSeconds = dt / 1000.0f;
    anims.Update(dtSeconds);

    // Sincronizar posición con el cuerpo físico
    int x, y;
    if (pbody)
    {
        pbody->GetPosition(x, y);
        position.setX((float)x);
        position.setY((float)y);
    }
    else
    {
        x = (int)position.getX();
        y = (int)position.getY();
    }

    SDL_Rect frame = anims.GetCurrentFrame();

    Engine::GetInstance().render->DrawTexture(
        texture,
        x - texW / 2,
        y - texH / 2,
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