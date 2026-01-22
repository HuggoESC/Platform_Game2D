#pragma once

#include "Entity.h"
#include "Animation.h"
#include "SDL3/SDL.h"
#include <box2d/box2d.h>
#include "Physics.h" 
#include <random>

enum class EnemyKind
{
	GROUND,   // tu slime actual
	FLYING,    // el nuevo enemigo volador
	BOSS       // El nuevo boss Cthulhu
};

enum class BossState
{
	IDLE,      // Not aware of player
	COMBAT,    // Aware of player, deciding action
	ATTACK_1,  // First attack pattern
	ATTACK_2,  // Second attack pattern
	FLYING,    // Flying/dodging
	DEAD       // Death state
};

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

	bool Start() override;
	bool CleanUp() override;

	bool flyFacingLeft = true;
	bool Update(float dt) override; // dt en milisegundos
	void OnCollision(PhysBody* physA, PhysBody* physB) override; // dt en milisegundos
	bool Destroy() override;

	void SetPosition(int x, int y);
	void MakeFlying(int frameW = 79, int frameH = 69);
	void MakeBoss(); // Nueva funcion para crear al boss

	bool IsBoss() const { return kind == EnemyKind::BOSS; }

	// Handle being hit by an attack
	void OnAttackHit();

	// Check if enemy is hit by attack at given position and range
	bool IsHitByAttack(float attackX, float attackY, float attackRange) const
	{
		float ex = position.getX();
		float ey = position.getY();
		float dx = attackX - ex;
		float dy = attackY - ey;
		float dist = std::sqrt(dx * dx + dy * dy);
		return dist <= attackRange;
	}

private:
	AnimationSet animations; // Conjunto de animaciones del enemigo
	SDL_Texture* texture = nullptr; // Textura del enemigo
	PhysBody* pbody = nullptr; // Cuerpo f?sico del enemigo
	PhysBody* sensorFront = nullptr; // Sensor frontal
	PhysBody* sensorBack = nullptr; // Sensor trasero
	float sensorOffset =16.0f; // Distancia del sensor al centro del enemigo 

	float speed =1.5f; // pixels por segundo
	int direction =1; //1: derecha, -1: izquierda

	// Pathfinding / scanning state
	int targetRow = -1;
	int targetCol = -1;
	bool hasTarget = false;

	float scanInterval =500.0f; // ms between scans
	float scanTimer =0.0f;
	float reachThreshold =4.0f; // pixels threshold to consider target reached
	int detectionRadiusTiles =8; // tiles to search for player

	// Patrol randomized movement
	std::mt19937 rng;
	float nextMoveTimer =0.0f; // ms until next random move
	int moveDistanceTiles =0; // how many tiles to move this action
	int moveDirection =0; // -1 or +1
	bool isMovingRandom = false;
	float moveTargetX =0.0f; // world X target for random move
	// random interval range (ms)
	int minMoveIntervalMs =1000;
	int maxMoveIntervalMs =3000;

	// Patrol check: if stuck (X hasn't changed much in this interval) flip direction
	float patrolCheckInterval =1000.0f; // ms to check movement
	float patrolTimer =0.0f;
	float lastPatrolX =0.0f; // last recorded X for patrol check

	// Prevent immediate double-flip when hitting wall
	float patrolFlipCooldown =300.0f; // ms cooldown after a flip/collision
	float patrolFlipTimer =0.0f;

	EnemyKind kind = EnemyKind::GROUND;

	// --- Flying ---
	SDL_Texture* flyingIdleTex = nullptr;
	SDL_Texture* flyingTex = nullptr;     // si quieres usar FLYING.png como anim principal
	int flyFrameW = 79;
	int flyFrameH = 69;

	struct SimpleAnim
	{
		std::vector<SDL_Rect> frames;
		float fps = 10.0f;
		bool loop = true;
		float acc = 0.0f;
		int idx = 0;

		void Reset() { acc = 0.0f; idx = 0; }
		void Update(float dt)
		{
			if (frames.empty()) return;
			acc += dt;
			float step = 1.0f / fps;
			while (acc >= step)
			{
				acc -= step;
				idx++;
				if (idx >= (int)frames.size())
				{
					idx = loop ? 0 : (int)frames.size() - 1;
				}
			}
		}
		SDL_Rect GetFrame() const
		{
			if (frames.empty()) return SDL_Rect{ 0,0,0,0 };
			return frames[idx];
		}
	};

	SimpleAnim flyIdleAnim;
	SimpleAnim flyAnim;
	SimpleAnim* currentFlyAnim = nullptr;

	// movimiento volador
	float flyBaseY = 0.0f;
	float flyTime = 0.0f;
	float flyBobAmp = 10.0f;    // cuanto sube y baja
	float flyBobFreq = 3.0f;    // velocidad de bob
	float flySpeed = 80.0f;     // velocidad horizontal/diagonal
	float flyAggroRange = 260.0f;

	// Flying enemy collision slowdown
	bool isSlowedDown = false;
	float slowdownTimer = 0.0f;
	const float slowdownDuration = 1.0f; 
	const float slowdownMultiplier = 0.3f; 

	// --- Boss ---
	int bossWidth = 192;
	int bossHeight = 112;
	bool bossHitThisFrame = false;
	int bossHealth = 30;
	const int bossMaxHealth = 30;
	const int healthBarPixelsPerHealth = 6;
	const int healthBarHeight = 8;
	BossState bossState = BossState::IDLE;
	BossState nextBossState = BossState::IDLE;
	float stateTimer = 0.0f;
	float stateTransitionTime = 0.0f;  // 3-5 seconds before entering action state
	int playerDetectionRadiusTiles = 12;  // tiles to detect player
	bool bossAwareOfPlayer = false;
	float bossSpeed = 1.5f;  // pixels per second
	int bossDirection = 1;  // 1 for right, -1 for left
	float bossIdleMoveTimer = 0.0f;  // timer to change direction
	float bossIdleMoveInterval = 3.0f;  // change direction every 3 seconds

	// --- Boss Attack 1: Projectile ---
	PhysBody* attack1Projectile = nullptr;  // physics body for the projectile
	Vector2D attack1ProjectilePos;  // current position of projectile
	Vector2D attack1ProjectileDir;  // direction of projectile (normalized)
	float attack1ProjectileSpeed = 200.0f;  // pixels per second
	float attack1ProjectileDistance = 0.0f;  // distance traveled so far
	const float attack1ProjectileMaxDistance = 128.0f;  // 4 squares (4 * 32)
	bool attack1Active = false;  // is attack 1 currently active
	
	// Attack 1 phases
	bool attack1Approaching = false;  // boss is moving toward player
	float attack1WaitTimer = 0.0f;  // timer for waiting before attack
	const float attack1WaitDuration = 0.5f;  // 0.5 seconds before firing
	const float attack1Distance = 60.0f;  // maintain 70 pixels distance from player

	// --- Boss Flying State ---
	float flyingDuration = 3.0f;  // seconds to fly
	float flyingTimer = 0.0f;// timer for current flight
	Vector2D bossSpawnPosition;  // initial spawn position for fallback
	
	// Flying movement smoothing
	float bossCurrentVelocityX = 0.0f;  // current X velocity during flight
	float bossCurrentVelocityY = 0.0f;  // current Y velocity during flight
	float bossFlightAcceleration = 2.0f;  // pixels/sec² acceleration during flight
	float bossMaxFlightSpeed = 32.0f;  // max speed during flight (pixels/sec)

	// State timeout handling
	BossState lastBossState = BossState::IDLE;  // tracks previous state
	float stateUnchangedTimer = 0.0f;  // timer to detect stuck states
	const float stateTimeoutDuration = 6.0f;  // 6 seconds before forcing flight
};