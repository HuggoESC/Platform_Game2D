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

 // Posición initial lógica
 position.setX((float)x);
 position.setY((float)y);

 // Textura y animaciones
 texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
 animations.LoadFromTSX("Assets/Textures/slime.tsx",
 { {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
 animations.SetCurrent("walkR");

 // cuerpo físico
 pbody = Engine::GetInstance().physics->CreateCircle(x, y,14, DYNAMIC);
 b2Body_SetFixedRotation(pbody->body, true);
 pbody->ctype = ColliderType::ENEMY;

 // sensores delantero y trasero
 sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x +20, y,6,6, STATIC);
 sensorFront->ctype = ColliderType::SENSOR;

 sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x -20, y,6,6, STATIC);
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

bool Enemy::Start()
{
    // IMPORTANTÍSIMO: aquí ya existe shared_ptr<Enemy>, así que shared_from_this() es seguro.
    if (pbody)       pbody->listener = shared_from_this();
    if (sensorFront) sensorFront->listener = shared_from_this();
    if (sensorBack)  sensorBack->listener = shared_from_this();

    return true;
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

    static bool listenersSet = false;
    if (!listenersSet)
    {
        if (pbody) pbody->listener = shared_from_this();
        if (sensorFront) sensorFront->listener = shared_from_this();
        if (sensorBack) sensorBack->listener = shared_from_this();
        listenersSet = true;
    }

    if (kind == EnemyKind::BOSS)
    {
        // Update position from physics body if it exists
        if (pbody)
        {
            int px, py;
            pbody->GetPosition(px, py);
            position.setX((float)px);
            position.setY((float)py);
        }

        const float dtSec = dt / 1000.0f;  // Convert ms to seconds
        int bx = (int)position.getX();
        int by = (int)position.getY();

        // Update state timer
        stateTimer += dtSec;

        // --- PLAYER DETECTION (when in IDLE or COMBAT) ---
        if (bossState == BossState::IDLE || bossState == BossState::COMBAT)
        {
            Map* map = Engine::GetInstance().map.get();
            int bossRow = 0, bossCol = 0;
            map->WorldToMap(bx, by, bossRow, bossCol);

            int playerRow = -1, playerCol = -1;
            bossAwareOfPlayer = FindPlayerInRadius(bossRow, bossCol, map, playerDetectionRadiusTiles, playerRow, playerCol);

            // Transition from IDLE to COMBAT when player detected
            if (bossAwareOfPlayer && bossState == BossState::IDLE)
            {
                bossState = BossState::COMBAT;
                stateTimer = 0.0f;
                LOG("CTHULHU COMBAT");
            }
        }

        // --- IDLE STATE: Move left and right ---
        if (bossState == BossState::IDLE)
        {
            // Update move timer
            bossIdleMoveTimer += dtSec;

            // Change direction every N seconds
            if (bossIdleMoveTimer >= bossIdleMoveInterval)
            {
                bossIdleMoveTimer = 0.0f;
                bossDirection *= -1;  // Flip direction
            }

            // Move the boss using physics velocity (like the slime does)
            Engine::GetInstance().physics->SetXVelocity(pbody, bossDirection * bossSpeed);
        }
        else
        {
            // Stop moving when not in IDLE
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        // --- COMBAT STATE: Wait 3-5 seconds, then choose an attack ---
        if (bossState == BossState::COMBAT)
        {
            // First time in combat, decide transition time
            if (stateTransitionTime == 0.0f)
            {
                std::uniform_real_distribution<float> timeDist(3.0f, 5.0f);
                stateTransitionTime = timeDist(rng);
            }

            // Wait for transition time
            if (stateTimer >= stateTransitionTime)
            {
                // Choose random attack (1-3)
                std::uniform_int_distribution<int> actionDist(1, 3);
                int chosenAction = actionDist(rng);

                if (chosenAction == 1)
                {
                    bossState = BossState::ATTACK_1;
                    LOG("CTHULHU ATTACKS 1");
                }
                else if (chosenAction == 2)
                {
                    bossState = BossState::ATTACK_2;
                    LOG("CTHULHU ATTACKS 2");
                }
                else
                {
                    bossState = BossState::FLYING;
                    LOG("CTHULHU FLIES");
                }

                stateTimer = 0.0f;
                stateTransitionTime = 0.0f;  // Reset for next combat phase
            }

        // --- STATE TIMEOUT CHECK (only during COMBAT and attacks, NOT IDLE) ---
            if (bossState != lastBossState)
       {
  // State changed, reset timer
    lastBossState = bossState;
           stateUnchangedTimer = 0.0f;
         }
            else if (bossState == BossState::COMBAT)
       {
         // In COMBAT state, check for timeout
    stateUnchangedTimer += dtSec;

         // If stuck in COMBAT for 5 seconds, spawn minion and go to FLYING
           if (stateUnchangedTimer >= stateTimeoutDuration)
    {
        LOG("CTHULHU STUCK IN COMBAT FOR 5 SECONDS - FORCING FLIGHT");

           // Spawn a flying enemy at current boss location
auto flyer = std::make_shared<Enemy>(bx, by - 50);
      flyer->MakeFlying();
    Engine::GetInstance().entityManager->AddEntity(flyer);
        LOG("TIMEOUT: Flying minion spawned at (%.0f, %.0f)", (float)bx, (float)(by - 50));

          // Force transition to FLYING state
     bossState = BossState::FLYING;
        stateTimer = 0.0f;
         stateTransitionTime = 0.0f;
             stateUnchangedTimer = 0.0f;

        LOG("CTHULHU FORCED INTO FLIGHT");
         }
    }
        }

  // --- Check timeout for attack states ---
  if (bossState == BossState::ATTACK_1 || bossState == BossState::ATTACK_2)
     {
     if (bossState != lastBossState)
  {
        // State changed, reset timer
    lastBossState = bossState;
   stateUnchangedTimer = 0.0f;
    }
            else
      {
   // Attack state unchanged, increment timeout timer
       stateUnchangedTimer += dtSec;

  // If stuck in attack state for 5 seconds, spawn minion and go to FLYING
        if (stateUnchangedTimer >= stateTimeoutDuration)
    {
    LOG("CTHULHU STUCK IN ATTACK STATE FOR 5 SECONDS - FORCING FLIGHT");

       // Spawn a flying enemy at current boss location
           auto flyer = std::make_shared<Enemy>(bx, by - 50);
            flyer->MakeFlying();
    Engine::GetInstance().entityManager->AddEntity(flyer);
        LOG("TIMEOUT: Flying minion spawned at (%.0f, %.0f)", (float)bx, (float)(by - 50));

      // Force transition to FLYING state
   bossState = BossState::FLYING;
        stateTimer = 0.0f;
      stateTransitionTime = 0.0f;
            stateUnchangedTimer = 0.0f;

           LOG("CTHULHU FORCED INTO FLIGHT");
           }
  }
        }

        // --- ATTACK STATES (placeholder for now) ---
        // These will be implemented later with actual attack patterns
        if (bossState == BossState::ATTACK_1)
     {
 // Initialize attack on first frame - use a flag to track if we've initialized
static bool attack1Initialized = false;
            if (!attack1Initialized)
       {
              attack1Initialized = true;
      attack1Approaching = true;
                attack1WaitTimer = 0.0f;
       attack1Active = false;
             attack1ProjectileDistance = 0.0f;
        
         LOG("ATTACK 1: Start approaching player");
   }

            // Find player position
  Vector2D playerPos = Vector2D(0.0f, 0.0f);
            bool foundPlayer = false;
            for (const auto& entPtr : Engine::GetInstance().entityManager->entities)
       {
              if (entPtr && entPtr->type == EntityType::PLAYER)
          {
  playerPos = entPtr->position;
      foundPlayer = true;
        break;
       }
    }

            if (foundPlayer)
    {
                // Phase 1: Move toward player until 70 pixels away
      if (attack1Approaching)
         {
       float distToPlayer = fabsf(playerPos.getX() - bx);
           
                // Calculate direction toward player
      int moveDir = (playerPos.getX() > bx) ? 1 : -1;
                    
        // Check if we're close enough
if (distToPlayer <= attack1Distance)
     {
         // Stop moving, start waiting
        attack1Approaching = false;
    attack1WaitTimer = 0.0f;
          Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
    LOG("ATTACK 1: Reached distance, waiting 0.5s before attack");
         }
       else
           {
        // Move toward player at boss speed
Engine::GetInstance().physics->SetXVelocity(pbody, moveDir * bossSpeed);
           }
}
   // Phase 2: Wait 0.5 seconds then fire projectile
      else
           {
         attack1WaitTimer += dtSec;
      
      if (attack1WaitTimer >= attack1WaitDuration)
           {
   // Time to attack! Fire projectile
       if (!attack1Active)
    {
        attack1Active = true;
          attack1ProjectileDistance = 0.0f;
   
          // Calculate direction (only horizontal, toward player)
     float dirX = (playerPos.getX() > bx) ? 1.0f : -1.0f;
              attack1ProjectileDir.setX(dirX);
           attack1ProjectileDir.setY(0.0f);
  
              // Spawn projectile at boss center
       attack1ProjectilePos.setX((float)bx);
  attack1ProjectilePos.setY((float)by);
       
      // Create projectile physics body
         int projW = 48;
          int projH = 48;
        attack1Projectile = Engine::GetInstance().physics->CreateRectangleSensor(
          (int)attack1ProjectilePos.getX(),
         (int)attack1ProjectilePos.getY(),
          projW,
            projH,
   KINEMATIC
   );
 attack1Projectile->ctype = ColliderType::UNKNOWN;
         attack1Projectile->listener = shared_from_this();
    
       LOG("ATTACK 1: Projectile fired at (%.0f, %.0f), direction %.0f", 
   attack1ProjectilePos.getX(), attack1ProjectilePos.getY(), dirX);
         }
                    }
 }
      }

          // Update projectile movement if active
       if (attack1Active && attack1Projectile != nullptr)
   {
       float moveAmount = attack1ProjectileSpeed * dtSec;
      attack1ProjectileDistance += moveAmount;

     // Move projectile
    attack1ProjectilePos.setX(attack1ProjectilePos.getX() + attack1ProjectileDir.getX() * moveAmount);
        attack1ProjectilePos.setY(attack1ProjectilePos.getY() + attack1ProjectileDir.getY() * moveAmount);

           // Update physics body position
    attack1Projectile->SetPosition(
       (int)attack1ProjectilePos.getX(),
          (int)attack1ProjectilePos.getY()
     );

 // Draw the projectile
             SDL_Rect projRect = {
        (int)attack1ProjectilePos.getX() - 24,
    (int)attack1ProjectilePos.getY() - 24,
        48,
  48
           };
      Engine::GetInstance().render->DrawRectangle(projRect, 255, 255, 0, 200, true, true);

          // Check collision with player
         if (foundPlayer)
             {
     float dx = attack1ProjectilePos.getX() - playerPos.getX();
 float dy = attack1ProjectilePos.getY() - playerPos.getY();
          float dist = sqrtf(dx * dx + dy * dy);

      if (dist <= 40.0f)
           {
    LOG("ATTACK 1: Hit player!");
  attack1ProjectileDistance = attack1ProjectileMaxDistance + 32.0f;
         }
            }

             // Check if projectile has traveled max distance
   if (attack1ProjectileDistance >= (attack1ProjectileMaxDistance + 32.0f))
                {
       LOG("ATTACK 1: Projectile disappeared");
        attack1Active = false;

               if (attack1Projectile)
            {
        Engine::GetInstance().physics->DeletePhysBody(attack1Projectile);
          attack1Projectile = nullptr;
    }
    }
  }

            // After attack finishes and projectile is gone, return to combat
         if (!attack1Active && attack1WaitTimer >= attack1WaitDuration + 0.2f)
            {
        bossState = BossState::COMBAT;
   stateTimer = 0.0f;
         stateTransitionTime = 0.0f;
        attack1Initialized = false;
    attack1Approaching = false;
  LOG("CTHULHU COMBAT");
  }
        }

        if (bossState == BossState::ATTACK_2)
        {
            // Initialize attack on first frame
            static bool attack2Initialized = false;
            if (!attack2Initialized)
            {
                attack2Initialized = true;
                LOG("ATTACK 2: Spawning minions!");

                // Spawn position: in front of boss based on direction
                int spawnOffsetX = 100 * bossDirection; // 100 pixels in front
                int spawnX = bx + spawnOffsetX;
                int spawnY = by;

                // Check if there's space to spawn enemies (basic bounds check)
                Map* map = Engine::GetInstance().map.get();
                int mapWidth = map->GetWidth() * map->GetTileWidth();
                
                if (spawnX > 0 && spawnX < mapWidth)
                {
                    // Spawn a slime enemy
                    auto slime = std::make_shared<Enemy>(spawnX, spawnY);
                    Engine::GetInstance().entityManager->AddEntity(slime);
                    LOG("ATTACK 2: Slime spawned at (%.0f, %.0f)", (float)spawnX, (float)spawnY);

                    // Spawn a flying enemy - offset should also respect direction
                    int flyerOffsetX = 50 * bossDirection; // offset in same direction as boss direction
                    auto flyer = std::make_shared<Enemy>(spawnX + flyerOffsetX, spawnY - 50);
                    flyer->MakeFlying();
                    Engine::GetInstance().entityManager->AddEntity(flyer);
                    LOG("ATTACK 2: Flying enemy spawned at (%.0f, %.0f)", (float)(spawnX + flyerOffsetX), (float)(spawnY - 50));
                }
                else
                {
                    LOG("ATTACK 2: Not enough space to spawn minions");
                }
            }

            // After a duration, return to COMBAT
            if (stateTimer > 1.0f)
            {
                bossState = BossState::COMBAT;
                stateTimer = 0.0f;
                stateTransitionTime = 0.0f;
                attack2Initialized = false;  // Reset for next attack
                LOG("CTHULHU COMBAT");
            }
        }

        if (bossState == BossState::FLYING)
        {
            // Initialize flying on first frame
     static bool flyingInitialized = false;
if (!flyingInitialized)
            {
  flyingInitialized = true;
           flyingTimer = 0.0f;
    
        // Disable collisions by converting to kinematic body
      // Save current position before changing body type
         int bx_temp = (int)position.getX();
     int by_temp = (int)position.getY();
     
                // Delete current body and recreate as kinematic (no collisions)
                Engine::GetInstance().physics->DeletePhysBody(pbody);
                pbody = Engine::GetInstance().physics->CreateRectangle(
             bx_temp, by_temp,
      bossWidth,
     bossHeight,
             KINEMATIC  // Kinematic bodies don't collide with terrain
       );
      pbody->ctype = ColliderType::ENEMY;
         pbody->listener = shared_from_this();
    b2Body_SetFixedRotation(pbody->body, true);
      
        // Remove gravity during flight
       b2Body_SetGravityScale(pbody->body, 0.0f);
      
        // Reset velocity tracking
                bossCurrentVelocityX = 0.0f;
   bossCurrentVelocityY = 0.0f;
   
       // Store spawn position if not already stored
     if (bossSpawnPosition.getX() == 0.0f && bossSpawnPosition.getY() == 0.0f)
    {
              bossSpawnPosition = position;
  }
  
                LOG("CTHULHU STARTS FLYING - COLLISIONS DISABLED");
      }
       
    // Increase flying timer
            flyingTimer += dtSec;
     
            // Find player position
     Vector2D playerPos = Vector2D((float)bx, (float)by);
            bool foundPlayer = false;
     for (const auto& entPtr : Engine::GetInstance().entityManager->entities)
            {
     if (entPtr && entPtr->type == EntityType::PLAYER)
   {
          playerPos = entPtr->position;
     foundPlayer = true;
          break;
         }
      }

       // Calculate target position: always above the player, maintain horizontal distance
            Vector2D targetPos;
      if (foundPlayer)
       {
        // Boss always stays above player (160 pixels above)
        // Horizontal position oscillates left/right using sine wave
         float horizontalOffset = sinf(flyingTimer * 2.0f) * 80.0f;  // -80 to +80 pixels side-to-side
   
          targetPos.setX(playerPos.getX() + horizontalOffset);
       targetPos.setY(playerPos.getY() - 160.0f);  // Always 160 pixels above
            }
            else
      {
     // No player found, fly upward from current position
         targetPos.setX((float)bx);
         targetPos.setY((float)by - 160.0f);
     }

    // Calculate desired direction and velocity
        Vector2D directionToTarget;
        directionToTarget.setX(targetPos.getX() - bx);
     directionToTarget.setY(targetPos.getY() - by);
 
            float distToTarget = sqrtf(directionToTarget.getX() * directionToTarget.getX() + 
       directionToTarget.getY() * directionToTarget.getY());
      
            if (distToTarget > 0.1f)
   {
   // Normalize direction
     directionToTarget.setX(directionToTarget.getX() / distToTarget);
   directionToTarget.setY(directionToTarget.getY() / distToTarget);
    
                // Calculate desired velocity (accelerate toward target)
         float desiredSpeedX = directionToTarget.getX() * bossMaxFlightSpeed;
   float desiredSpeedY = directionToTarget.getY() * bossMaxFlightSpeed;

          // Smooth acceleration toward desired velocity
      float accelAmount = bossFlightAcceleration * dtSec;
      
  // Accelerate X
         if (bossCurrentVelocityX < desiredSpeedX)
      {
       bossCurrentVelocityX = fminf(bossCurrentVelocityX + accelAmount, desiredSpeedX);
           }
   else if (bossCurrentVelocityX > desiredSpeedX)
             {
    bossCurrentVelocityX = fmaxf(bossCurrentVelocityX - accelAmount, desiredSpeedX);
     }
            
         // Accelerate Y
           if (bossCurrentVelocityY < desiredSpeedY)
  {
            bossCurrentVelocityY = fminf(bossCurrentVelocityY + accelAmount, desiredSpeedY);
      }
      else if (bossCurrentVelocityY > desiredSpeedY)
    {
         bossCurrentVelocityY = fmaxf(bossCurrentVelocityY - accelAmount, desiredSpeedY);
        }
         }
    else
            {
           // Very close to target, decelerate to stop
       float decelAmount = bossFlightAcceleration * dtSec * 0.5f;

      if (fabs(bossCurrentVelocityX) > 0.1f)
   {
   bossCurrentVelocityX *= (1.0f - decelAmount * dtSec);
          }
     else
      {
   bossCurrentVelocityX = 0.0f;
       }
   
         if (fabs(bossCurrentVelocityY) > 0.1f)
                {
     bossCurrentVelocityY *= (1.0f - decelAmount * dtSec);
 }
       else
       {
     bossCurrentVelocityY = 0.0f;
         }
       }
        
            // Apply smoothed velocity to boss
      Engine::GetInstance().physics->SetLinearVelocity(pbody, bossCurrentVelocityX, bossCurrentVelocityY);
         
 // Check if flight duration is finished
            if (flyingTimer >= flyingDuration)
            {
     LOG("CTHULHU FLIGHT FINISHED - ATTEMPTING LANDING");
  
 // Recreate as DYNAMIC body with collisions restored
     int bx_land = (int)position.getX();
       int by_land = (int)position.getY();
      
      Engine::GetInstance().physics->DeletePhysBody(pbody);
  pbody = Engine::GetInstance().physics->CreateRectangle(
        bx_land, by_land,
      bossWidth,
   bossHeight,
        DYNAMIC  // Back to dynamic for collisions
                );
           pbody->ctype = ColliderType::ENEMY;
       pbody->listener = shared_from_this();
                b2Body_SetFixedRotation(pbody->body, true);
      
     // Restore gravity
          b2Body_SetGravityScale(pbody->body, 1.0f);
 
          // Reset velocity for smooth landing
   bossCurrentVelocityX = 0.0f;
       bossCurrentVelocityY = 0.0f;
    
            // Try to find ground below current position
      int rayStartX = bx_land;
         int rayStartY = by_land;
            int rayEndX = bx_land;
                int rayEndY = by_land + 1000;  // raycast downward for up to 1000 pixels
    
    float normalX = 0.0f, normalY = 0.0f;
  int hitDistance = pbody->RayCast(rayStartX, rayStartY, rayEndX, rayEndY, normalX, normalY);
    
             if (hitDistance >= 0 && hitDistance < 1000)
   {
     // Found ground! Land safely
    int safeY = rayStartY + hitDistance - 10;  // buffer of 10 pixels
              pbody->SetPosition(rayStartX, safeY);
       position.setX((float)rayStartX);
      position.setY((float)safeY);
     
      // Apply small downward velocity to make landing natural
    Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 1.0f);
    
         LOG("CTHULHU LANDED AT (%.0f, %.0f)", (float)rayStartX, (float)safeY);
         }
      else
                {
  // No ground found - spawn flying enemy and continue flying
          LOG("CTHULHU NO GROUND FOUND - SPAWNING MINION AND FLYING AGAIN");

         // Spawn a flying enemy at boss location
       auto flyer = std::make_shared<Enemy>(bx_land, by_land - 50);
      flyer->MakeFlying();
    Engine::GetInstance().entityManager->AddEntity(flyer);
           LOG("Flying minion spawned at (%.0f, %.0f)", (float)bx_land, (float)(by_land - 50));
     
               // Restart flight - convert back to kinematic
         flyingTimer = 0.0f;
            bossCurrentVelocityX = 0.0f;
     bossCurrentVelocityY = 0.0f;
  
           Engine::GetInstance().physics->DeletePhysBody(pbody);
           pbody = Engine::GetInstance().physics->CreateRectangle(
   bx_land, by_land,
  bossWidth,
   bossHeight,
       KINEMATIC
          );
         pbody->ctype = ColliderType::ENEMY;
    pbody->listener = shared_from_this();
       b2Body_SetFixedRotation(pbody->body, true);
b2Body_SetGravityScale(pbody->body, 0.0f);
         
       LOG("CTHULHU FLIGHT RESTARTED");
       }
     
          // Reset flying state and return to combat after landing attempt
    bossState = BossState::COMBAT;
         stateTimer = 0.0f;
  stateTransitionTime = 0.0f;
flyingInitialized = false;  // Reset for next flight
            }
        }

        // --- DEATH STATE ---
        if (bossHealth <= 0 && bossState != BossState::DEAD)
        {
            bossState = BossState::DEAD;
            LOG("CTHULHU DIES");
        }

        // --- DRAW BOSS ---
        // Update position from physics body before drawing
        if (pbody)
        {
            pbody->GetPosition(bx, by);
        }

        // Draw the 192x112 rectangle centered at position
        SDL_Rect bossRect = {
            bx - bossWidth / 2,
            by - bossHeight / 2,
            bossWidth,
            bossHeight
        };

        // Draw filled rectangle with semi-transparent red
        Engine::GetInstance().render->DrawRectangle(bossRect, 200, 50, 50, 200, true, true);
        // Draw border
        Engine::GetInstance().render->DrawRectangle(bossRect, 255, 0, 0, 255, false, true);

        // Draw health bar
        // Position: 64 pixels upwards and 92 pixels to the left from middle of hitbox
        int healthBarX = bx - 92;
        int healthBarY = by - 64;

        // Health bar width based on current health
        int healthBarWidth = bossHealth * healthBarPixelsPerHealth;

        SDL_Rect healthBar = {
            healthBarX,
            healthBarY,
            healthBarWidth,
            healthBarHeight
        };

        // Draw health bar background (dark red)
        SDL_Rect healthBarBG = {
            healthBarX,
            healthBarY,
            bossMaxHealth * healthBarPixelsPerHealth,
            healthBarHeight
        };
        Engine::GetInstance().render->DrawRectangle(healthBarBG, 50, 20, 20, 255, true, true);

        // Draw health bar foreground (green)
        Engine::GetInstance().render->DrawRectangle(healthBar, 0, 255, 0, 255, true, true);

        // Draw health bar border
        Engine::GetInstance().render->DrawRectangle(healthBarBG, 255, 255, 255, 255, false, true);

        return true;
    }

    if (kind == EnemyKind::FLYING)
    {
        // dt llega en MILISEGUNDOS en tu proyecto -> lo pasamos a segundos
        const float dtSec = dt / 1000.0f;
        static bool facingLeft = true;

        // Update slowdown timer
        if (isSlowedDown)
        {
            slowdownTimer -= dtSec;
            if (slowdownTimer <= 0.0f)
            {
                isSlowedDown = false;
            }
        }

        // Tiempo para bob y animación (solo 1 vez por frame)
        flyTime += dtSec;

        // Posición del enemigo en PIXELES (para dibujar) y también en METROS (para Box2D)
        int px, py;
        pbody->GetPosition(px, py);

        // Actualizar position con los valores del pbody para que el hitbox se sincronice
        position.setX((float)px);
        position.setY((float)py);

        const b2Vec2 posM = b2Body_GetPosition(pbody->body);
        float xM = posM.x;
        float yM = posM.y;

        // Si por lo que sea flyBaseY está a 0 o no inicializado, lo inicializamos (EN METROS)
        if (flyBaseY == 0.0f || flyBaseY < 0.1f)
            flyBaseY = yM;

        // Bob en Y (flyBobAmp está en PÍXELES -> lo convertimos a METROS)
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
                // entPtr->position está en PÍXELES -> convertimos a METROS
                playerXM = PIXEL_TO_METERS(entPtr->position.getX());
                playerYM = PIXEL_TO_METERS(entPtr->position.getY());
                hasPlayer = true;
                break;
            }
        }

        // Por defecto: hover (se mantiene cerca del targetYM)
        float vx = 0.0f;
        float vy = (targetYM - yM) * 3.0f;

        // Si hay jugador, persecución si está en rango
        if (hasPlayer)
        {
            // flyAggroRange está en PÍXELES -> a METROS
            const float aggroRangeM = PIXEL_TO_METERS(500.0f);

            // Queremos apuntar un poco por encima del jugador para que no "choque raro"
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
                float chaseSpeed = 3.5f;
                
                // Apply slowdown multiplier if slowed down
                if (isSlowedDown)
                {
                    chaseSpeed *= -slowdownMultiplier;
                }

                vx = nx * chaseSpeed;

                if (fabsf(vx) > 0.05f)
                {
                    facingLeft = (vx < 0.0f);
				}

                // Perseguir al jugador en Y sin restricciones de bob
                vy = ny * chaseSpeed;

                currentFlyAnim = &flyAnim;       // si no tienes frames de flyAnim, puedes dejar idle
            }
            else
            {
                // Volver a hovering cuando sale del rango
                vy = (targetYM - yM) * 3.0f;
                currentFlyAnim = &flyIdleAnim;
            }
        }
        else
        {
            // Sin jugador: hovering normal
            currentFlyAnim = &flyIdleAnim;
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
	// Ignore collisions between slimes/minions and other enemies (but NOT boss-to-enemy)
	// The boss (which is also ENEMY type) should collide with platforms but not minions
	if (physA->ctype == ColliderType::ENEMY && physB->ctype == ColliderType::ENEMY)
	{
		// Only filter if BOTH are regular enemies (not boss)
		// Boss will be checked below with kind == BOSS
		if (kind != EnemyKind::BOSS && physB->listener.lock() && 
		    dynamic_cast<Enemy*>(physB->listener.lock().get()) &&
		    dynamic_cast<Enemy*>(physB->listener.lock().get())->IsBoss() == false)
		{
			// Regular enemy colliding with another regular enemy - ignore
			return;
		}
	}

	// Boss collision with attacks - ONLY take damage from PLAYER attacks (not own projectiles)
	// Note: Damage is handled through OnAttackHit() in Player.cpp, NOT through collision
	if (kind == EnemyKind::BOSS && physB->ctype == ColliderType::PLAYER)
	{
		// Boss damage is handled elsewhere, not here
		return;
	}

	// Consideramos sólido todo lo que no sea jugador, sensor o desconocido
	bool bloqueSolido =
	physB->ctype != ColliderType::PLAYER &&
	physB->ctype != ColliderType::SENSOR &&
	physB->ctype != ColliderType::UNKNOWN &&
	physB->ctype != ColliderType::ENEMY;

	// Solo nos interesa cuándo el QUE choca es el cuerpo del slime
 if (physA == pbody && bloqueSolido)
 {
 direction *= -1; // invertimos dirección
 hasTarget = false; // cancel current target when hitting obstacle
 isMovingRandom = false; // cancel random move on collision
 patrolFlipTimer = patrolFlipCooldown; // avoid immediate flip back
 LOG("SLIME: cambio de dirección");
 }

 // Flying enemy collision with player - slow down for 2 seconds
 if (kind == EnemyKind::FLYING && physA == pbody && physB->ctype == ColliderType::PLAYER)
 {
 isSlowedDown = true;
 slowdownTimer = slowdownDuration;
 LOG("FLYING ENEMY: Slowed down after hitting player!");
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
    pbody->ctype = ColliderType::ENEMY;

	pbody->listener = shared_from_this();

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

    // Base Y para el bob - inicializar a la posición actual del cuerpo
    b2Vec2 p = b2Body_GetPosition(pbody->body);
    flyBaseY = p.y; // EN METROS - usar la posición actual como base

    LOG("Enemy convertido a FLYING con %d frames, flyBaseY initialized to %.2f meters", (int)flyIdleAnim.frames.size(), flyBaseY);
}

void Enemy::MakeBoss()
{
	kind = EnemyKind::BOSS;

	// Create a rectangle hitbox for the boss
	int bx, by;
	if (pbody)
	{
		pbody->GetPosition(bx, by);
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	else
	{
		bx = (int)position.getX();
		by = (int)position.getY();
	}

	// Store spawn position for flying fallback
	bossSpawnPosition.setX((float)bx);
	bossSpawnPosition.setY((float)by);

	// Create boss as a regular body that collides with ground but not other enemies
	// Use CreateRectangle (not sensor) so it respects gravity and collides with platforms
	pbody = Engine::GetInstance().physics->CreateRectangle(
		bx, by,
		bossWidth,
		bossHeight,
		DYNAMIC
	);
	pbody->ctype = ColliderType::ENEMY;
	pbody->listener = shared_from_this();

	// Keep gravity enabled and fixed rotation
	b2Body_SetFixedRotation(pbody->body, true);

	// Remove old sensors if they exist
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

	LOG("Boss created at position (%.0f, %.0f) with hitbox %dx%d", position.getX(), position.getY(), bossWidth, bossHeight);
}

void Enemy::OnAttackHit()
{
	if (kind == EnemyKind::BOSS)
	{
		LOG("CTHULHU HIT!");
		bossHealth--;
		if (bossHealth < 0) bossHealth = 0;
		LOG("Boss health: %d / %d", bossHealth, bossMaxHealth);
	}
}

bool Enemy::CleanUp()
{
    // Clean up attack 1 projectile if it exists
    if (attack1Projectile)
    {
        Engine::GetInstance().physics->DeletePhysBody(attack1Projectile);
        attack1Projectile = nullptr;
    }

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