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

	position.setX((float)x);
	position.setY((float)y);

	texture = Engine::GetInstance().textures->Load("Assets/Textures/slime.png");
	animations.LoadFromTSX("Assets/Textures/slime.tsx",
	{ {0,"idle"}, {4,"walkL"}, {14,"jump"}, {28,"walkR"}, {38,"dead"} });
	animations.SetCurrent("walkR");

	pbody = Engine::GetInstance().physics->CreateCircle(x, y,14, DYNAMIC);
	b2Body_SetFixedRotation(pbody->body, true);
	pbody->ctype = ColliderType::ENEMY;

	sensorFront = Engine::GetInstance().physics->CreateRectangleSensor(x +20, y,6,6, STATIC);
	sensorFront->ctype = ColliderType::SENSOR;

	sensorBack = Engine::GetInstance().physics->CreateRectangleSensor(x -20, y,6,6, STATIC);
	sensorBack->ctype = ColliderType::SENSOR;

	lastPatrolX = (float)x;

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
	if (pbody)       pbody->listener = shared_from_this();
	if (sensorFront) sensorFront->listener = shared_from_this();
	if (sensorBack)  sensorBack->listener = shared_from_this();

	return true;
}

static bool IsInside(int r, int c, int rows, int cols) {
	return r >=0 && c >=0 && r < rows && c < cols;
}

static bool BFS_FindWalkTarget(Map* map, int startRow, int startCol, int maxRadius, int maxClimbPx, int& outRow, int& outCol)
{
	int rows = map->GetHeight();
	int cols = map->GetWidth();
	int tileH = map->GetTileHeight();

	std::vector<uint8_t> visited(rows * cols,0);
	std::queue<std::tuple<int,int,int>> q;
	q.push({startRow, startCol,0});
	visited[startRow * cols + startCol] =1;

	const int dr[4] = {0,0, -1,1};
	const int dc[4] = {-1,1,0,0};

	while (!q.empty()) {
		auto front = q.front(); q.pop();
		int r = std::get<0>(front);
		int c = std::get<1>(front);
		int d = std::get<2>(front);

		if (d >0) {
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

			if (map->IsCollisionTileAt(nr, nc)) {
				visited[nr * cols + nc] =1;
				continue;
			}

			int vertPx = (startRow - nr) * tileH;
			if (vertPx > maxClimbPx) {
				visited[nr * cols + nc] =1;
				continue;
			}

			visited[nr * cols + nc] =1;
			q.push({nr, nc, d +1});
		}
	}

	return false;
}

static bool FindPlayerInRadius(int startRow, int startCol, Map* map, int radius, int& outRow, int& outCol)
{
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
		if (pbody)
		{
			int px, py;
			pbody->GetPosition(px, py);
			position.setX((float)px);
			position.setY((float)py);
		}

		const float dtSec = dt / 1000.0f;
		int bx = (int)position.getX();
		int by = (int)position.getY();

		stateTimer += dtSec;

		if (bossState == BossState::IDLE || bossState == BossState::COMBAT)
		{
			Map* map = Engine::GetInstance().map.get();
			int bossRow = 0, bossCol = 0;
			map->WorldToMap(bx, by, bossRow, bossCol);

			int playerRow = -1, playerCol = -1;
			bossAwareOfPlayer = FindPlayerInRadius(bossRow, bossCol, map, playerDetectionRadiusTiles, playerRow, playerCol);

			if (bossAwareOfPlayer && bossState == BossState::IDLE)
			{
				bossState = BossState::COMBAT;
				stateTimer = 0.0f;
				LOG("CTHULHU COMBAT");
			}
		}

		if (bossState == BossState::IDLE)
		{
			bossIdleMoveTimer += dtSec;

			if (bossIdleMoveTimer >= bossIdleMoveInterval)
			{
				bossIdleMoveTimer = 0.0f;
				bossDirection *= -1;
			}

			Engine::GetInstance().physics->SetXVelocity(pbody, bossDirection * bossSpeed);
		}
		else
		{
			Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
		}

		if (bossState == BossState::COMBAT)
		{
			if (stateTransitionTime == 0.0f)
			{
				std::uniform_real_distribution<float> timeDist(3.0f, 5.0f);
				stateTransitionTime = timeDist(rng);
			}

			if (stateTimer >= stateTransitionTime)
			{
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
				stateTransitionTime = 0.0f;
			}

			if (bossState != lastBossState)
			{
				lastBossState = bossState;
				stateUnchangedTimer = 0.0f;
			}
			else if (bossState == BossState::COMBAT)
			{
				stateUnchangedTimer += dtSec;

				if (stateUnchangedTimer >= stateTimeoutDuration)
				{
					LOG("CTHULHU STUCK IN COMBAT FOR 5 SECONDS - FORCING FLIGHT");

					auto flyer = std::make_shared<Enemy>(bx, by - 50);
					flyer->MakeFlying();
					Engine::GetInstance().entityManager->AddEntity(flyer);
					LOG("TIMEOUT: Flying minion spawned at (%.0f, %.0f)", (float)bx, (float)(by - 50));

					bossState = BossState::FLYING;
					stateTimer = 0.0f;
					stateTransitionTime = 0.0f;
					stateUnchangedTimer = 0.0f;

					LOG("CTHULHU FORCED INTO FLIGHT");
				}
			}
		}

		if (bossState == BossState::ATTACK_1 || bossState == BossState::ATTACK_2)
		{
			if (bossState != lastBossState)
			{
				lastBossState = bossState;
				stateUnchangedTimer = 0.0f;
			}
			else
			{
				stateUnchangedTimer += dtSec;

				if (stateUnchangedTimer >= stateTimeoutDuration)
				{
					LOG("CTHULHU STUCK IN ATTACK STATE FOR 5 SECONDS - FORCING FLIGHT");

					auto flyer = std::make_shared<Enemy>(bx, by - 50);
					flyer->MakeFlying();
					Engine::GetInstance().entityManager->AddEntity(flyer);
					LOG("TIMEOUT: Flying minion spawned at (%.0f, %.0f)", (float)bx, (float)(by - 50));

					bossState = BossState::FLYING;
					stateTimer = 0.0f;
					stateTransitionTime = 0.0f;
					stateUnchangedTimer = 0.0f;

					LOG("CTHULHU FORCED INTO FLIGHT");
				}
			}
		}

		if (bossState == BossState::ATTACK_1)
		{
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
				if (attack1Approaching)
				{
					float distToPlayer = fabsf(playerPos.getX() - bx);

					int moveDir = (playerPos.getX() > bx) ? 1 : -1;

					if (distToPlayer <= attack1Distance)
					{
						attack1Approaching = false;
						attack1WaitTimer = 0.0f;
						Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
						LOG("ATTACK 1: Reached distance, waiting 0.5s before attack");
					}
					else
					{
						Engine::GetInstance().physics->SetXVelocity(pbody, moveDir * bossSpeed);
					}
				}
				else
				{
					attack1WaitTimer += dtSec;

					if (attack1WaitTimer >= attack1WaitDuration)
					{
						if (!attack1Active)
						{
							attack1Active = true;
							attack1ProjectileDistance = 0.0f;

							float dirX = (playerPos.getX() > bx) ? 1.0f : -1.0f;
							attack1ProjectileDir.setX(dirX);
							attack1ProjectileDir.setY(0.0f);

							attack1ProjectilePos.setX((float)bx);
							attack1ProjectilePos.setY((float)by);

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

			if (attack1Active && attack1Projectile != nullptr)
			{
				float moveAmount = attack1ProjectileSpeed * dtSec;
				attack1ProjectileDistance += moveAmount;

				attack1ProjectilePos.setX(attack1ProjectilePos.getX() + attack1ProjectileDir.getX() * moveAmount);
				attack1ProjectilePos.setY(attack1ProjectilePos.getY() + attack1ProjectileDir.getY() * moveAmount);

				attack1Projectile->SetPosition(
					(int)attack1ProjectilePos.getX(),
					(int)attack1ProjectilePos.getY()
				);

				SDL_Rect projRect = {
					(int)attack1ProjectilePos.getX() - 24,
					(int)attack1ProjectilePos.getY() - 24,
					48,
					48
				};
				Engine::GetInstance().render->DrawRectangle(projRect, 255, 255, 0, 200, true, true);

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
			static bool attack2Initialized = false;
			if (!attack2Initialized)
			{
				attack2Initialized = true;
				LOG("ATTACK 2: Spawning minions!");

				int spawnOffsetX = 100 * bossDirection;
				int spawnX = bx + spawnOffsetX;
				int spawnY = by;

				Map* map = Engine::GetInstance().map.get();
				int mapWidth = map->GetWidth() * map->GetTileWidth();

				if (spawnX > 0 && spawnX < mapWidth)
				{
					auto slime = std::make_shared<Enemy>(spawnX, spawnY);
					Engine::GetInstance().entityManager->AddEntity(slime);
					LOG("ATTACK 2: Slime spawned at (%.0f, %.0f)", (float)spawnX, (float)spawnY);

					int flyerOffsetX = 50 * bossDirection;
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

			if (stateTimer > 1.0f)
			{
				bossState = BossState::COMBAT;
				stateTimer = 0.0f;
				stateTransitionTime = 0.0f;
				attack2Initialized = false;
				LOG("CTHULHU COMBAT");
			}
		}

		if (bossState == BossState::FLYING)
		{
			static bool flyingInitialized = false;
			if (!flyingInitialized)
			{
				flyingInitialized = true;
				flyingTimer = 0.0f;

				int bx_temp = (int)position.getX();
				int by_temp = (int)position.getY();

				Engine::GetInstance().physics->DeletePhysBody(pbody);
				pbody = Engine::GetInstance().physics->CreateRectangle(
					bx_temp, by_temp,
					bossWidth,
					bossHeight,
					KINEMATIC
				);
				pbody->ctype = ColliderType::ENEMY;
				pbody->listener = shared_from_this();
				b2Body_SetFixedRotation(pbody->body, true);

				b2Body_SetGravityScale(pbody->body, 0.0f);

				bossCurrentVelocityX = 0.0f;
				bossCurrentVelocityY = 0.0f;

				if (bossSpawnPosition.getX() == 0.0f && bossSpawnPosition.getY() == 0.0f)
				{
					bossSpawnPosition = position;
				}

				LOG("CTHULHU STARTS FLYING - COLLISIONS DISABLED");
			}

			flyingTimer += dtSec;

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

			Vector2D targetPos;
			if (foundPlayer)
			{
				float horizontalOffset = sinf(flyingTimer * 2.0f) * 80.0f;

				targetPos.setX(playerPos.getX() + horizontalOffset);
				targetPos.setY(playerPos.getY() - 160.0f);
			}
			else
			{
				targetPos.setX((float)bx);
				targetPos.setY((float)by - 160.0f);
			}

			Vector2D directionToTarget;
			directionToTarget.setX(targetPos.getX() - bx);
			directionToTarget.setY(targetPos.getY() - by);

			float distToTarget = sqrtf(directionToTarget.getX() * directionToTarget.getX() +
				directionToTarget.getY() * directionToTarget.getY());

			if (distToTarget > 0.1f)
			{
				directionToTarget.setX(directionToTarget.getX() / distToTarget);
				directionToTarget.setY(directionToTarget.getY() / distToTarget);

				float desiredSpeedX = directionToTarget.getX() * bossMaxFlightSpeed;
				float desiredSpeedY = directionToTarget.getY() * bossMaxFlightSpeed;

				float accelAmount = bossFlightAcceleration * dtSec;

				if (bossCurrentVelocityX < desiredSpeedX)
				{
					bossCurrentVelocityX = fminf(bossCurrentVelocityX + accelAmount, desiredSpeedX);
				}
				else if (bossCurrentVelocityX > desiredSpeedX)
				{
					bossCurrentVelocityX = fmaxf(bossCurrentVelocityX - accelAmount, desiredSpeedX);
				}

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

			Engine::GetInstance().physics->SetLinearVelocity(pbody, bossCurrentVelocityX, bossCurrentVelocityY);

			if (flyingTimer >= flyingDuration)
			{
				LOG("CTHULHU FLIGHT FINISHED - ATTEMPTING LANDING");

				int bx_land = (int)position.getX();
				int by_land = (int)position.getY();

				Engine::GetInstance().physics->DeletePhysBody(pbody);
				pbody = Engine::GetInstance().physics->CreateRectangle(
					bx_land, by_land,
					bossWidth,
					bossHeight,
					DYNAMIC
				);
				pbody->ctype = ColliderType::ENEMY;
				pbody->listener = shared_from_this();
				b2Body_SetFixedRotation(pbody->body, true);

				b2Body_SetGravityScale(pbody->body, 1.0f);

				bossCurrentVelocityX = 0.0f;
				bossCurrentVelocityY = 0.0f;

				int rayStartX = bx_land;
				int rayStartY = by_land;
				int rayEndX = bx_land;
				int rayEndY = by_land + 1000;

				float normalX = 0.0f, normalY = 0.0f;
				int hitDistance = pbody->RayCast(rayStartX, rayStartY, rayEndX, rayEndY, normalX, normalY);

				if (hitDistance >= 0 && hitDistance < 1000)
				{
					int safeY = rayStartY + hitDistance - 10;
					pbody->SetPosition(rayStartX, safeY);
					position.setX((float)rayStartX);
					position.setY((float)safeY);

					Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 1.0f);

					LOG("CTHULHU LANDED AT (%.0f, %.0f)", (float)rayStartX, (float)safeY);
				}
				else
				{
					LOG("CTHULHU NO GROUND FOUND - SPAWNING MINION AND FLYING AGAIN");

					auto flyer = std::make_shared<Enemy>(bx_land, by_land - 50);
					flyer->MakeFlying();
					Engine::GetInstance().entityManager->AddEntity(flyer);
					LOG("Flying minion spawned at (%.0f, %.0f)", (float)bx_land, (float)(by_land - 50));

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

				bossState = BossState::COMBAT;
				stateTimer = 0.0f;
				stateTransitionTime = 0.0f;
				flyingInitialized = false;
			}
		}

		if (bossHealth <= 0 && bossState != BossState::DEAD)
		{
			bossState = BossState::DEAD;
			LOG("CTHULHU DIES");
		}

		if (pbody)
		{
			pbody->GetPosition(bx, by);
		}

		SDL_Rect bossRect = {
			bx - bossWidth / 2,
			by - bossHeight / 2,
			bossWidth,
			bossHeight
		};

		Engine::GetInstance().render->DrawRectangle(bossRect, 200, 50, 50, 200, true, true);
		Engine::GetInstance().render->DrawRectangle(bossRect, 255, 0, 0, 255, false, true);

		int healthBarX = bx - 92;
		int healthBarY = by - 64;

		int healthBarWidth = bossHealth * healthBarPixelsPerHealth;

		SDL_Rect healthBar = {
			healthBarX,
			healthBarY,
			healthBarWidth,
			healthBarHeight
		};

		SDL_Rect healthBarBG = {
			healthBarX,
			healthBarY,
			bossMaxHealth * healthBarPixelsPerHealth,
			healthBarHeight
		};
		Engine::GetInstance().render->DrawRectangle(healthBarBG, 50, 20, 20, 255, true, true);

		Engine::GetInstance().render->DrawRectangle(healthBar, 0, 255, 0, 255, true, true);

		Engine::GetInstance().render->DrawRectangle(healthBarBG, 255, 255, 255, 255, false, true);

		return true;
	}

	if (kind == EnemyKind::FLYING)
	{
		const float dtSec = dt / 1000.0f;
		static bool facingLeft = true;

		if (isSlowedDown)
		{
			slowdownTimer -= dtSec;
			if (slowdownTimer <= 0.0f)
			{
				isSlowedDown = false;
			}
		}

		flyTime += dtSec;

		int px, py;
		pbody->GetPosition(px, py);

		position.setX((float)px);
		position.setY((float)py);

		const b2Vec2 posM = b2Body_GetPosition(pbody->body);
		float xM = posM.x;
		float yM = posM.y;

		if (flyBaseY == 0.0f || flyBaseY < 0.1f)
			flyBaseY = yM;

		const float bobM = PIXEL_TO_METERS(flyBobAmp) * sinf(flyTime * flyBobFreq);
		const float targetYM = flyBaseY + bobM;

		bool hasPlayer = false;
		float playerXM = xM;
		float playerYM = yM;

		for (const auto& entPtr : Engine::GetInstance().entityManager->entities)
		{
			if (entPtr && entPtr->type == EntityType::PLAYER)
			{
				playerXM = PIXEL_TO_METERS(entPtr->position.getX());
				playerYM = PIXEL_TO_METERS(entPtr->position.getY());
				hasPlayer = true;
				break;
			}
		}

		float vx = 0.0f;
		float vy = (targetYM - yM) * 3.0f;

		if (hasPlayer)
		{
			const float aggroRangeM = PIXEL_TO_METERS(500.0f);

			const float aimOffsetY = PIXEL_TO_METERS(20.0f);

			float dx = playerXM - xM;
			flyFacingLeft = (dx < 0.0f);
			float dy = (playerYM - aimOffsetY) - yM;
			float dist = sqrtf(dx * dx + dy * dy);

			if (dist < aggroRangeM && dist > 0.001f)
			{
				float nx = dx / dist;
				float ny = dy / dist;

				float chaseSpeed = 3.5f;

				if (isSlowedDown)
				{
					chaseSpeed *= -slowdownMultiplier;
				}

				vx = nx * chaseSpeed;

				if (fabsf(vx) > 0.05f)
				{
					facingLeft = (vx < 0.0f);
				}

				vy = ny * chaseSpeed;

				currentFlyAnim = &flyAnim;
			}
			else
			{
				vy = (targetYM - yM) * 3.0f;
				currentFlyAnim = &flyIdleAnim;
			}
		}
		else
		{
			currentFlyAnim = &flyIdleAnim;
		}

		Engine::GetInstance().physics->SetLinearVelocity(pbody, vx, vy);

		if (currentFlyAnim != nullptr)
			currentFlyAnim->Update(dtSec);

		if (flyingTex == nullptr || currentFlyAnim == nullptr)
		{
			SDL_Rect frame = animations.GetCurrentFrame();
			Engine::GetInstance().render->DrawTexture(texture, px - 16, py - 16, &frame);
		}
		else
		{
			SDL_Texture* texToDraw = (currentFlyAnim == &flyAnim) ? flyingTex : flyingIdleTex;
			SDL_Rect fr = currentFlyAnim->GetFrame();

			bool flipX = !flyFacingLeft;

			Engine::GetInstance().render->DrawTexture(
				texToDraw,
				px - (flyFrameW / 2),
				py - (flyFrameH / 2),
				&fr,
				1.0f,
				0.0,
				INT_MAX,
				INT_MAX,
				flipX
			);
		}

		return true;
	}

	animations.Update(dt);

	int px, py;
	pbody->GetPosition(px, py);

	position.setX((float)px);
	position.setY((float)py);

	if (sensorFront) sensorFront->SetPosition(px + (int)sensorOffset * direction, py);
	if (sensorBack) sensorBack->SetPosition(px - (int)sensorOffset * direction, py);

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

	int prow = -1, pcol = -1;
	bool playerFound = FindPlayerInRadius(startRow, startCol, map, detectionRadiusTiles, prow, pcol);

	if (playerFound) {
		targetRow = prow;
		targetCol = pcol;
		hasTarget = true;

		isMovingRandom = false;

		std::uniform_int_distribution<int> dist(minMoveIntervalMs, maxMoveIntervalMs);
		nextMoveTimer = (float)dist(rng);

	} else {
		if (!isMovingRandom) {
			nextMoveTimer -= dt;
			if (nextMoveTimer <=0.0f) {
				std::uniform_int_distribution<int> dirDist(0,1);
				moveDirection = dirDist(rng) ?1 : -1;
				std::uniform_int_distribution<int> distTiles(1,3);
				moveDistanceTiles = distTiles(rng);

				int tgtCol = startCol + moveDirection * moveDistanceTiles;
				int tgtRow = startRow;

				if (tgtCol <0) tgtCol =0;
				if (tgtCol >= map->GetWidth()) tgtCol = map->GetWidth() -1;

				bool ok = true;
				if (map->IsCollisionTileAt(tgtRow, tgtCol)) ok = false;
				if (!(tgtRow +1 < map->GetHeight() && map->IsCollisionTileAt(tgtRow +1, tgtCol))) ok = false;

				if (ok) {
					Vector2D tw = map->MapToWorld(tgtRow, tgtCol);
					moveTargetX = tw.getX() + tileW /2.0f;
					isMovingRandom = true;
				} else {
					std::uniform_int_distribution<int> tdist(minMoveIntervalMs, maxMoveIntervalMs);
					nextMoveTimer = (float)tdist(rng);
				}
			}
		}
	}

	if (hasTarget) {
		Vector2D targetWorld = map->MapToWorld(targetRow, targetCol);
		float targetX = targetWorld.getX() + tileW /2.0f;
		float dx = targetX - px;

		if (std::abs(dx) < reachThreshold) {
			hasTarget = false;
			Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
		} else {
			direction = (dx >0.0f) ?1 : -1;
			Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);
		}
	} else if (isMovingRandom) {
		float dx = moveTargetX - px;
		if (std::abs(dx) < reachThreshold) {
			isMovingRandom = false;
			Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
			std::uniform_int_distribution<int> dist(minMoveIntervalMs, maxMoveIntervalMs);
			nextMoveTimer = (float)dist(rng);
		} else {
			direction = (dx >0.0f) ?1 : -1;
			Engine::GetInstance().physics->SetXVelocity(pbody, direction * speed);
		}
	} else {
		Engine::GetInstance().physics->SetXVelocity(pbody,0.0f);
	}

	if (patrolFlipTimer >0.0f) patrolFlipTimer -= dt;

	SDL_Rect frame = animations.GetCurrentFrame();
	Engine::GetInstance().render->DrawTexture(texture, px -16, py -16, &frame);

	return true;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physA->ctype == ColliderType::ENEMY && physB->ctype == ColliderType::ENEMY)
	{
		if (kind != EnemyKind::BOSS && physB->listener.lock() && 
		    dynamic_cast<Enemy*>(physB->listener.lock().get()) &&
		    dynamic_cast<Enemy*>(physB->listener.lock().get())->IsBoss() == false)
		{
			return;
		}
	}

	if (kind == EnemyKind::BOSS && physB->ctype == ColliderType::PLAYER)
	{
		return;
	}

	bool bloqueSolido =
	physB->ctype != ColliderType::PLAYER &&
	physB->ctype != ColliderType::SENSOR &&
	physB->ctype != ColliderType::UNKNOWN &&
	physB->ctype != ColliderType::ENEMY;

	if (physA == pbody && bloqueSolido)
	{
		direction *= -1;
		hasTarget = false;
		isMovingRandom = false;
		patrolFlipTimer = patrolFlipCooldown;
		LOG("SLIME: cambio de dirección");
	}

	if (kind == EnemyKind::FLYING && physA == pbody && physB->ctype == ColliderType::PLAYER)
	{
		isSlowedDown = true;
		slowdownTimer = slowdownDuration;
		LOG("FLYING ENEMY: Slowed down after hitting player!");
	}
}

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

	int bx, by;
	pbody->GetPosition(bx, by);

	Engine::GetInstance().physics->DeletePhysBody(pbody);
	pbody = nullptr;

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

	flyAnim.fps = 12.0f;
	flyAnim.loop = true;
	flyAnim.Reset();

	currentFlyAnim = &flyIdleAnim;

	b2Body_SetGravityScale(pbody->body, 0.0f);
	Engine::GetInstance().physics->SetYVelocity(pbody, 0.0f);

	b2Vec2 p = b2Body_GetPosition(pbody->body);
	flyBaseY = p.y;

	LOG("Enemy convertido a FLYING con %d frames, flyBaseY initialized to %.2f meters", (int)flyIdleAnim.frames.size(), flyBaseY);
}

void Enemy::MakeBoss()
{
	kind = EnemyKind::BOSS;

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

	bossSpawnPosition.setX((float)bx);
	bossSpawnPosition.setY((float)by);

	pbody = Engine::GetInstance().physics->CreateRectangle(
		bx, by,
		bossWidth,
		bossHeight,
		DYNAMIC
	);
	pbody->ctype = ColliderType::ENEMY;
	pbody->listener = shared_from_this();

	b2Body_SetFixedRotation(pbody->body, true);

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
	if (attack1Projectile)
	{
		Engine::GetInstance().physics->DeletePhysBody(attack1Projectile);
		attack1Projectile = nullptr;
	}

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