#include "hoguera.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "Scene.h"

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
    std::unordered_map<int, std::string> aliases = {
     {0, "on"},   // fuego animado (tile id 0)
     {5, "off"}   // apagada (tile id 5) <-- AJUSTA EL 5 si tu TSX usa otro id
    };
    anims.LoadFromTSX("Assets/Textures/hoguera.tsx", aliases);
    anims.SetCurrent("off");  // empieza apagada

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

    pbody->ctype = ColliderType::SENSOR; 
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
    if (activated) return;

    if (physB->ctype == ColliderType::PLAYER)
    {
        activated = true;

		anims.SetCurrent("on");

        Engine::GetInstance().scene->SetCheckpoint(position);

        // Si quieres un sonido al activar:
        // Engine::GetInstance().audio->PlayFx(fxCheckpointId);
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