#pragma once
#include "Entity.h"
#include "Physics.h"
#include "Vector2D.h"

class MapChangeTrigger : public Entity
{
public:
    MapChangeTrigger(int x, int y, int w, int h, int targetLevel, int spawnX, int spawnY);
    ~MapChangeTrigger() override = default;

    bool Start() override;
    bool Update(float dt) override { return true; }
    void OnCollision(PhysBody* physA, PhysBody* physB) override;
    bool CleanUp() override;

private:
    int x, y, w, h;
    int targetLevel;
    Vector2D targetSpawn;
    PhysBody* pbody = nullptr;
    bool used = false;
};

