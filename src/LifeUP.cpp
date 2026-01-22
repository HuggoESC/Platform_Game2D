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

    SDL_Rect frame = anims.GetCurrentFrame();
    int fw = frame.w;
    int fh = frame.h;

    int cx = (int)position.getX() + fw / 2;
    int cy = (int)position.getY() + fh / 2;

    pbody = Engine::GetInstance().physics->CreateRectangleSensor(cx, cy, fw, fh, bodyType::STATIC);

    pbody->listener = shared_from_this();
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

    if (!active) return true;

    active = false;

    
    if (pbody)
    {
        pbody->listener.reset();
        pbody->pendingDelete = true;
    }

    Engine::GetInstance().entityManager->DestroyEntity(shared_from_this());
    return true;
}