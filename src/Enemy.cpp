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

#include <queue>
#include <tuple>
#include <vector>
#include <cmath>
#include <random>

Enemy::Enemy(int x, int y)
{
 type = EntityType::ENEMY;

 // Posición inicial lógica
 position.setX((float)x);
 position.setY((float)y);

 // Textura y animaciones
 texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
 animations.LoadFromTSX("Assets/Textures/slime.tsx",
 { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
 animations.SetCurrent("walkR");

 // cuerpo físico
 pbody = Engine::GetInstance().physics->CreateCircle(x, y,14, DYNAMIC);
 pbody->listener = shared_from_this();
 b2Body_SetFixedRotation(pbody->body, true);
 pbody->ctype = ColliderType::ENEMY;

 // sensores delantero y trasero
 sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x +20, y,6,6, STATIC);
 pbody->listener = shared_from_this();
 sensorFront->ctype = ColliderType::SENSOR;

 sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x -20, y,6,6, STATIC);
 pbody->listener = shared_from_this();
 sensorBack->ctype = ColliderType::SENSOR;

 // initialize patrol last position
 lastPatrolX = (float)x;

 // init RNG and first move timer
 std::random_device rd;
 rng = std::mt19937(rd());
 std::uniform_int_distribution<int> dist(minMoveIntervalMs, maxMoveIntervalMs);
 nextMoveTimer = (float)dist(rng);
}

Enemy::~Enemy()
{
}

static bool IsInside(int r, int c, int rows, int cols) {
 return r >=0 && c >=0 && r < rows && c < cols;
}

// BFS helper: find nearest reachable tile within radius that has ground below
// Returns true and fills targetRow/targetCol when found.
static bool BFS_FindWalkTarget(Map* map, int startRow, int startCol, int maxRadius, int maxClimbPx, int& outRow, int& outCol)
{
 int rows = map->GetHeight();
 int cols = map->GetWidth();
 int tileH = map->GetTileHeight();

 std::vector<uint8_t> visited(rows * cols,0);
 std::queue<std::tuple<int,int,int>> q; // row, col, depth
 q.push({startRow, startCol,0});
 visited[startRow * cols + startCol] =1;

 const int dr[4] = {0,0, -1,1}; // left,right,up,down (cols change for left/right)
 const int dc[4] = {-1,1,0,0};

 while (!q.empty()) {
 auto front = q.front(); q.pop();
 int r = std::get<0>(front);
 int c = std::get<1>(front);
 int d = std::get<2>(front);

 if (d >0) {
 // candidate tile: must be empty (no collision) and have ground underneath (collision)
 if (!map->IsCollisionTileAt(r, c)) {
 int belowR = r +1;
 if (belowR < rows && map->IsCollisionTileAt(belowR, c)) {
 outRow = r;
 outCol = c;
 return true;
 }
 }
 }

 if (d >= maxRadius) continue;

 for (int k =0; k <4; ++k) {
 int nr = r + dr[k];
 int nc = c + dc[k];

 if (!IsInside(nr, nc, rows, cols)) continue;
 if (visited[nr * cols + nc]) continue;

 // Stop scanning direction if target tile is a collision (platform)
 if (map->IsCollisionTileAt(nr, nc)) {
 // mark visited to avoid reprocessing but do not enqueue
 visited[nr * cols + nc] =1;
 continue;
 }

 // Check climb constraint: how many pixels above startRow is this tile
 int vertPx = (startRow - nr) * tileH; // positive if nr is above startRow
 if (vertPx > maxClimbPx) {
 // too high climb, skip
 visited[nr * cols + nc] =1;
 continue;
 }

 visited[nr * cols + nc] =1;
 q.push({nr, nc, d +1});
 }
 }

 return false;
}

// Helper to find player entity within radius (in tiles) from startRow/startCol
static bool FindPlayerInRadius(int startRow, int startCol, Map* map, int radius, int& outRow, int& outCol)
{
 // iterate entities and find player position
 for (const auto& entPtr : Engine::GetInstance().entityManager->entities) {
 if (entPtr->type == EntityType::PLAYER) {
 int px = (int)entPtr->position.getX();
 int py = (int)entPtr->position.getY();
 int prow, pcol;
 map->WorldToMap(px, py, prow, pcol);

 int dr = std::abs(prow - startRow);
 int dc = std::abs(pcol - startCol);
 if (dr <= radius && dc <= radius) {
 outRow = prow;
 outCol = pcol;
 return true;
 }
 }
 }
 return false;
}

bool Enemy::Update(float dt)
{

    // ---------------- FLYING ENEMY UPDATE ----------------
   // ---------------- FLYING ENEMY UPDATE ----------------
    if (kind == EnemyKind::FLYING)
    {
        // dt llega en MILISEGUNDOS en tu proyecto -> lo pasamos a segundos
        const float dtSec = dt / 1000.0f;
        static bool facingLeft = true;

        // Tiempo para bob y animación (solo 1 vez por frame)
        flyTime += dtSec;

        // Posición del enemigo en PIXELES (para dibujar) y también en METROS (para Box2D)
        int px, py;
        pbody->GetPosition(px, py);

        const b2Vec2 posM = b2Body_GetPosition(pbody->body);
        float xM = posM.x;
        float yM = posM.y;

        // Si por lo que sea flyBaseY está a 0 (poco probable), lo inicializamos (EN METROS)
        if (flyBaseY == 0.0f)
            flyBaseY = yM;

        // Bob en Y (flyBobAmp está en PIXELES -> lo convertimos a METROS)
        const float bobM = PIXEL_TO_METERS(flyBobAmp) * sinf(flyTime * flyBobFreq);
        const float targetYM = flyBaseY + bobM;

        // --- Buscar player ---
        bool hasPlayer = false;
        float playerXM = xM;
        float playerYM = yM;

        for (const auto& entPtr : Engine::GetInstance().entityManager->entities)
        {
            if (entPtr && entPtr->type == EntityType::PLAYER)
            {
                // entPtr->position está en PIXELES -> convertimos a METROS
                playerXM = PIXEL_TO_METERS(entPtr->position.getX());
                playerYM = PIXEL_TO_METERS(entPtr->position.getY());
                hasPlayer = true;
                break;
            }
        }

        // Por defecto: hover (se mantiene cerca del targetYM)
        float vx = 0.0f;
        float vy = (targetYM - yM) * 3.0f;

        // Si hay jugador, perseguir si está en rango
        if (hasPlayer)
        {
            // flyAggroRange está en PIXELES -> a METROS
            const float aggroRangeM = PIXEL_TO_METERS(500.0f);

            // Queremos apuntar un poco por encima del jugador para que no “choque raro”
            const float aimOffsetY = PIXEL_TO_METERS(20.0f);

            float dx = playerXM - xM;
			flyFacingLeft = (dx < 0.0f);
            float dy = (playerYM - aimOffsetY) - yM;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < aggroRangeM && dist > 0.001f)
            {
                float nx = dx / dist;
                float ny = dy / dist;

                // OJO: velocidades de Box2D están en METROS/SEG.
                // Tu flySpeed actual (80) era enorme si lo interpretamos como m/s.
                // Vamos a usar una velocidad razonable (2.5 m/s ~ 125 px/s).
                const float chaseSpeed = 3.5f;

                vx = nx * chaseSpeed;

                if (fabsf(vx) > 0.05f)
                {
                    facingLeft = (vx < 0.0f);
				}

                // Mezclamos persecución con el bob para que no “tiemble”
                float chaseVy = ny * chaseSpeed;
                float bobVy = (targetYM - yM) * 2.0f;
                vy = chaseVy + bobVy;

                currentFlyAnim = &flyAnim;       // si no tienes frames de flyAnim, puedes dejar idle
            }
            else
            {
                currentFlyAnim = &flyIdleAnim;
            }
        }

        // Aplicar velocidad REAL al body (en vez de SetPosition cada frame)
        Engine::GetInstance().physics->SetLinearVelocity(pbody, vx, vy);

        // Actualizar animación con dtSec
        if (currentFlyAnim != nullptr)
            currentFlyAnim->Update(dtSec);

        // --- DIBUJO ---
        if (flyingTex == nullptr || currentFlyAnim == nullptr)
        {
            // Fallback si algo falla
            SDL_Rect frame = animations.GetCurrentFrame();
            Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);
        }
        else
        {
            SDL_Texture* texToDraw = (currentFlyAnim == &flyAnim) ? flyingTex : flyingIdleTex;
            SDL_Rect fr = currentFlyAnim->GetFrame();

            // orientación según movimiento
            bool flipX = !flyFacingLeft;

            Engine::GetInstance().render->DrawTexture(
                texToDraw,
                px - (flyFrameW / 2),
                py - (flyFrameH / 2),
                &fr,
                1.0f,      // speed
                0.0,       // angle
                INT_MAX,   // pivotX
                INT_MAX,   // pivotY
                flipX
            );
        }

        return true;
    }

 animations.Update(dt);

 int px, py;
 pbody->GetPosition(px, py);

 // Update position from physics body
 position.setX((float)px);
 position.setY((float)py);

 // Update sensors to follow the body
 if (sensorFront) sensorFront->SetPosition(px + (int)sensorOffset * direction, py);
 if (sensorBack) sensorBack->SetPosition(px - (int)sensorOffset * direction, py);

 // perform periodic scanning
 scanTimer += dt;
 bool shouldScan = false;
 if (scanTimer >= scanInterval) {
 scanTimer =0.0f;
 shouldScan = true;
 }

 Map* map = Engine::GetInstance().map.get();
 int tileW = map->GetTileWidth();
 int tileH = map->GetTileHeight();

 int startRow =0, startCol =0;
 map->WorldToMap(px, py, startRow, startCol);

 // Patrol check: see if stuck
 int prow = -1, pcol = -1;
 bool playerFound = FindPlayerInRadius(startRow, startCol, map, detectionRadiusTiles, prow, pcol);

 if (playerFound) {
 // set target to player's tile (always update directly, chase)
 targetRow = prow;
 targetCol = pcol;
 hasTarget = true;

 // cancel any random move
 isMovingRandom = false;

 // reset timers used for random moves
 std::uniform_int_distribution<int> dist(minMoveIntervalMs, maxMoveIntervalMs);
 nextMoveTimer = (float)dist(rng);

 } else {
 // No player: perform randomized moves on timer
 if (!isMovingRandom) {
 nextMoveTimer -= dt;
 if (nextMoveTimer <=0.0f) {
 // choose random direction and distance
 std::uniform_int_distribution<int> dirDist(0,1);
 moveDirection = dirDist(rng) ?1 : -1;
 std::uniform_int_distribution<int> distTiles(1,3);
 moveDistanceTiles = distTiles(rng);

 // compute candidate target tile
 int tgtCol = startCol + moveDirection * moveDistanceTiles;
 int tgtRow = startRow;

 // bounds check
 if (tgtCol <0) tgtCol =0;
 if (tgtCol >= map->GetWidth()) tgtCol = map->GetWidth() -1;

 // Check that target tile is empty and has ground below (so enemy won't fall)
 bool ok = true;
 // target tile should be empty
 if (map->IsCollisionTileAt(tgtRow, tgtCol)) ok = false;
 // and there should be a collision tile below target
 if (!(tgtRow +1 < map->GetHeight() && map->IsCollisionTileAt(tgtRow +1, tgtCol))) ok = false;

 if (ok) {
 Vector2D tw = map->MapToWorld(tgtRow, tgtCol);
 moveTargetX = tw.getX() + tileW /2.0f;
 isMovingRandom = true;
 // set velocity toward target; will be handled below in movement block
 } else {
 // schedule next try
 std::uniform_int_distribution<int> tdist(minMoveIntervalMs, maxMoveIntervalMs);
 nextMoveTimer = (float)tdist(rng);
 }
 }
 }
 }

 // Movement handling
 if (hasTarget) {
 // chase player target
 Vector2D targetWorld = map->MapToWorld(targetRow, targetCol);
 float targetX = targetWorld.getX() + tileW /2.0f;
 float dx = targetX - px;

 if (std::abs(dx) < reachThreshold) {
 // reached
 hasTarget = false;
 Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
 } else {
 direction = (dx >0.0f) ?1 : -1;
 Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);
 }
 } else if (isMovingRandom) {
 float dx = moveTargetX - px;
 if (std::abs(dx) < reachThreshold) {
 // reached end of random move
 isMovingRandom = false;
 Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
 // schedule next random move
 std::uniform_int_distribution<int> dist(minMoveIntervalMs, maxMoveIntervalMs);
 nextMoveTimer = (float)dist(rng);
 } else {
 direction = (dx >0.0f) ?1 : -1;
 Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);
 }
 } else {
 // idle / small patrol - stand still until next random move
 Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
 }

 // update flip cooldown
 if (patrolFlipTimer >0.0f) patrolFlipTimer -= dt;

 SDL_Rect frame = animations.GetCurrentFrame();
 Engine::GetInstance().render->DrawTexture(texture, px -16, py -16, &frame);

 return true;
}

// Collision handling
void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
 // Consideramos sólido todo lo que no sea jugador, sensor o desconocido
 bool bloqueSolido =
 physB->ctype != ColliderType::PLAYER &&
 physB->ctype != ColliderType::SENSOR &&
 physB->ctype != ColliderType::UNKNOWN;

 // Solo nos interesa cuándo el QUE choca es el cuerpo del slime
 if (physA == pbody && bloqueSolido)
 {
 direction *= -1; // invertimos dirección
 hasTarget = false; // cancel current target when hitting obstacle
 isMovingRandom = false; // cancel random move on collision
 patrolFlipTimer = patrolFlipCooldown; // avoid immediate flip back
 LOG("SLIME: cambio de dirección");
 }
}

// Destroy enemy
bool Enemy::Destroy()
{
    LOG("Destroying enemy");
    active = false;
    Engine::GetInstance().entityManager->DestroyEntity(shared_from_this());
    return true;
}

void Enemy::SetPosition(int x, int y)
{
    position.setX((float)x);
    position.setY((float)y);

    if (pbody)
        pbody->SetPosition(x, y);

    // Si tienes sensores, también los movemos para que no se queden atrás
    if (sensorFront) sensorFront->SetPosition(x + (int)sensorOffset * direction, y);
    if (sensorBack)  sensorBack->SetPosition(x - (int)sensorOffset * direction, y);
}

void Enemy::MakeFlying(int frameW, int frameH)
{
    kind = EnemyKind::FLYING;
    flyFrameW = frameW;
    flyFrameH = frameH;

    if (sensorFront)
    {
        Engine::GetInstance().physics->DeletePhysBody(sensorFront);
        sensorFront = nullptr;
    }
    if (sensorBack)
    {
        Engine::GetInstance().physics->DeletePhysBody(sensorBack);
        sensorBack = nullptr;
    }

    // Rehacemos el body como sensor (así no choca con suelo/bloques)
    int bx, by;
    pbody->GetPosition(bx, by);

    Engine::GetInstance().physics->DeletePhysBody(pbody);
    pbody = nullptr;

    // Un sensor rectangular suele ir mejor que un círculo para sprites
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(
        bx, by,
        (int)(flyFrameW * 0.45f),
        (int)(flyFrameH * 0.45f),
        DYNAMIC
    );
    pbody->listener = shared_from_this();
    pbody->ctype = ColliderType::ENEMY;

    b2Body_SetFixedRotation(pbody->body, true);
    b2Body_SetGravityScale(pbody->body, 0.0f);

    // Cargamos sprite del volador (usa el que quieras: IDLE.png o FLYING.png)
    // Te recomiendo empezar por IDLE.png
    flyingIdleTex = Engine::GetInstance().textures->Load("Assets/Textures/IDLEDEMON.png");
    flyingTex = Engine::GetInstance().textures->Load("Assets/Textures/FLYINGDEMON.png");

    if (!flyingIdleTex)
    {
        LOG("ERROR: No se pudo cargar IDLEDEMON.png");
        return;
    }
    if (!flyingTex)
    {
        LOG("ERROR: No se pudo cargar FLYINGDEMON.png");
        return;
    }

    // -------------------- FRAMES IDLE --------------------
    flyIdleAnim.frames.clear();

    float idleW = 0.f, idleH = 0.f;
    SDL_GetTextureSize(flyingIdleTex, &idleW, &idleH);

    int idleCols = (int)(idleW / flyFrameW);
    for (int i = 0; i < idleCols; ++i)
    {
        SDL_Rect r;
        r.x = i * flyFrameW;
        r.y = 0;
        r.w = flyFrameW;
        r.h = flyFrameH;
        flyIdleAnim.frames.push_back(r);
    }

    flyIdleAnim.fps = 10.0f;
    flyIdleAnim.loop = true;
    flyIdleAnim.Reset();

    // -------------------- FRAMES FLYING (MOVIMIENTO) --------------------
    flyAnim.frames.clear();

    float flyW = 0.f, flyH = 0.f;
    SDL_GetTextureSize(flyingTex, &flyW, &flyH);

    int flyCols = (int)(flyW / flyFrameW);
    for (int i = 0; i < flyCols; ++i)
    {
        SDL_Rect r;
        r.x = i * flyFrameW;
        r.y = 0;
        r.w = flyFrameW;
        r.h = flyFrameH;
        flyAnim.frames.push_back(r);
    }

    flyAnim.fps = 12.0f;   // un pelín más dinámica
    flyAnim.loop = true;
    flyAnim.Reset();

    currentFlyAnim = &flyIdleAnim;

    // Quitar gravedad para que no caiga
    b2Body_SetGravityScale(pbody->body, 0.0f);
    Engine::GetInstance().physics->SetYVelocity(pbody, 0.0f);

    // Base Y para el bob
    b2Vec2 p = b2Body_GetPosition(pbody->body);
    flyBaseY = p.y; // EN METROS

    LOG("Enemy convertido a FLYING con %d frames", (int)flyIdleAnim.frames.size());
}

bool Enemy::CleanUp()
{
    // Evita callbacks a memoria liberada
    if (pbody)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }

    if (sensorFront)
    {
        Engine::GetInstance().physics->DeletePhysBody(sensorFront);
        sensorFront = nullptr;
    }

    if (sensorBack)
    {
        Engine::GetInstance().physics->DeletePhysBody(sensorBack);
        sensorBack = nullptr;
    }

    if (texture)
    {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    if (flyingIdleTex)
    {
        Engine::GetInstance().textures->UnLoad(flyingIdleTex);
        flyingIdleTex = nullptr;
    }
    if (flyingTex)
    {
        Engine::GetInstance().textures->UnLoad(flyingTex);
        flyingTex = nullptr;
    }

    return true;
}