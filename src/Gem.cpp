#include "Gem.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "EntityManager.h"

Gem::Gem(int x, int y) : Entity(EntityType::GEM)
{
    name = "Gem";
    position.setX((float)x);
    position.setY((float)y);
}

bool Gem::Awake()
{
    return true;
}

bool Gem::Start()
{
  texture = Engine::GetInstance().textures->Load("Assets/Textures/gem_round_32x32_12f_0d.png");
    Engine::GetInstance().textures->GetSize(texture, texW, texH);

    std::unordered_map<int, std::string> aliases = { {0, "idle"} };
    anims.LoadFromTSX("Assets/Maps/gem_round_32x32_12f_0d.tsx", aliases);
  anims.SetCurrent("idle");

    SDL_Rect frame = anims.GetCurrentFrame();
    int fw = frame.w;
    int fh = frame.h;

    int cx = (int)position.getX() + fw / 2;
    int cy = (int)position.getY() + fh / 2;

    pbody = Engine::GetInstance().physics->CreateRectangleSensor(cx, cy, fw, fh, bodyType::STATIC);
pbody->listener = shared_from_this();
    pbody->ctype = ColliderType::GEM;

    return true;
}

bool Gem::Update(float dt)
{
    if (!active) return true;

    anims.Update(dt);
 SDL_Rect frame = anims.GetCurrentFrame();

    int x, y;
    if (pbody)
        pbody->GetPosition(x, y);
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

void Gem::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physB && physB->ctype == ColliderType::PLAYER)
    {
  LOG("Gem touched by player");
    }
}

bool Gem::CleanUp()
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

bool Gem::Destroy()
{
    if (picked) return true;
    picked = true;

    LOG("Destroying Gem");

    active = false;

    if (pbody)
    {
        pbody->ctype = ColliderType::UNKNOWN;    
        pbody->pendingDelete = true;
    }

    Engine::GetInstance().entityManager->DestroyEntity(shared_from_this());
    return true;
}