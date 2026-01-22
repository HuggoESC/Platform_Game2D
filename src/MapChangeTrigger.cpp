#include "MapChangeTrigger.h"
#include "Engine.h"
#include "Scene.h"
#include "Log.h"

MapChangeTrigger::MapChangeTrigger(int x_, int y_, int w_, int h_, int targetLevel_, int spawnX, int spawnY)
    : Entity(EntityType::UNKNOWN), x(x_), y(y_), w(w_), h(h_), targetLevel(targetLevel_), targetSpawn((float)spawnX, (float)spawnY)
{
    name = "MapChangeTrigger";
}

bool MapChangeTrigger::Start()
{
    // Ojo: en TMX x,y suele venir como top-left. Convertimos a centro:
    int cx = x + w / 2;
    int cy = y + h / 2;

    pbody = Engine::GetInstance().physics->CreateRectangleSensor(cx, cy, w, h, bodyType::STATIC);
    pbody->ctype = ColliderType::MAPCHANGE;
    pbody->listener = shared_from_this();

    return true;
}

void MapChangeTrigger::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (used) return;
    if (!physB) return;

    if (physB->ctype == ColliderType::PLAYER)
    {
        used = true;

        // Desarmar para que no dispare otra vez este frame
        if (pbody) pbody->listener.reset();

        LOG("MapChangeTrigger -> request level %d", targetLevel);

        // IMPORTANTÍSIMO: pedir cambio, NO cargar nivel directamente desde callback
        Engine::GetInstance().scene->RequestLevelChange(targetLevel, targetSpawn);
    }
}

bool MapChangeTrigger::CleanUp()
{
    if (pbody)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}