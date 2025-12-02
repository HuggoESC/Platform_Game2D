#include "Hoguera.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"

hoguera::hoguera(int x, int y)
    : Entity(EntityType::UNKNOWN)   // O hacer un nuevo EntityType::HOGUERA si luego quieres
{
    name = "Hoguera";

    position.setX((float)x);
    position.setY((float)y);

    // ?? CAMBIA la ruta si tu textura tiene otro nombre o está en otra carpeta
    texture = Engine::GetInstance().textures->Load("Assets/Textures/hoguera.png");

    // Creamos un sensor para detectar al jugador
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(
        x, y, 32, 32, STATIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::SENSOR;
}

bool hoguera::Update(float dt)
{
    // Dibujar la hoguera (ajusta el offset si se ve desplazada)
    Engine::GetInstance().render->DrawTexture(
        texture,
        (int)position.getX() - 16,
        (int)position.getY() - 16
    );

    return true;
}

void hoguera::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physA == pbody && physB->ctype == ColliderType::PLAYER)
    {
        LOG("?? HOGUERA: Jugador ha tocado la hoguera ? Guardando partida (aún fake)");
        // Aquí haremos el guardado real después
    }
}