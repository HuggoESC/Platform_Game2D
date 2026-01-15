#include "hoguera.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"

hoguera::hoguera(int x, int y)
    : Entity(EntityType::HOGUERA)
{
    name = "hoguera";
    position.setX((float)x);
    position.setY((float)y);
}

bool hoguera::Awake()
{
    return true;
}

bool hoguera::Start()
{
    // Animación
    std::unordered_map<int, std::string> aliases = { {0,"idle"} };
    anims.LoadFromTSX("Assets/Textures/hoguera.tsx", aliases);
    anims.SetCurrent("idle");

    // Textura
    texture = Engine::GetInstance().textures->Load("Assets/Textures/hoguera.png");

    texW = anims.GetCurrentFrame().w;
    texH = anims.GetCurrentFrame().h;

    // Sensor para detectar al jugador
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(
        (int)position.getX(),
        (int)position.getY(),
        texW,
        texH,
        STATIC
    );

    pbody->ctype = ColliderType::ITEM; 
    pbody->listener = this;

    return true;
}

bool hoguera::Update(float dt)
{
    anims.Update(dt);

    const SDL_Rect& frame = anims.GetCurrentFrame();

    Engine::GetInstance().render->DrawTexture(
        texture,
        (int)position.getX() - texW / 2,
        (int)position.getY() - texH / 2,
        &frame
    );

    return true;
}

void hoguera::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physB->ctype == ColliderType::PLAYER)
    {
        LOG("HOGUERA tocada por jugador");
        // Guardado lo haremos después
    }
}

bool hoguera::CleanUp()
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