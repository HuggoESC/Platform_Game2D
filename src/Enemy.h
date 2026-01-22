#pragma once

#include "Entity.h"
#include "Animation.h"
#include "SDL3/SDL.h"
#include <box2d/box2d.h>
#include "Physics.h" 
#include <random>

enum class EnemyKind
{
	GROUND,   
	FLYING,    
	BOSS      
};

enum class BossState
{
	IDLE,     
	COMBAT,    
	ATTACK_1,  
	ATTACK_2,  
	FLYING,    
	DEAD       
};

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

	bool Start() override;
	bool CleanUp() override;

	bool flyFacingLeft = true;
	bool Update(float dt) override; 
	void OnCollision(PhysBody* physA, PhysBody* physB) override; 
	bool Destroy() override;

	void SetPosition(int x, int y);
	void MakeFlying(int frameW = 79, int frameH = 69);
	void MakeBoss(); 

	bool IsBoss() const { return kind == EnemyKind::BOSS; }

	void OnAttackHit();

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
	AnimationSet animations; 
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr; 
	PhysBody* sensorFront = nullptr; 
	PhysBody* sensorBack = nullptr; 
	float sensorOffset =16.0f; 

	float speed =1.5f; 
	int direction =1; 

	int targetRow = -1;
	int targetCol = -1;
	bool hasTarget = false;

	float scanInterval =500.0f; 
	float scanTimer =0.0f;
	float reachThreshold =4.0f; 
	int detectionRadiusTiles =8; 

	std::mt19937 rng;
	float nextMoveTimer =0.0f; 
	int moveDistanceTiles =0; 
	int moveDirection =0; 
	bool isMovingRandom = false;
	float moveTargetX =0.0f; 
	int minMoveIntervalMs =1000;
	int maxMoveIntervalMs =3000;

	float patrolCheckInterval =1000.0f; 
	float patrolTimer =0.0f;
	float lastPatrolX =0.0f; 

	float patrolFlipCooldown =300.0f; 
	float patrolFlipTimer =0.0f;

	EnemyKind kind = EnemyKind::GROUND;

	SDL_Texture* flyingIdleTex = nullptr;
	SDL_Texture* flyingTex = nullptr;     
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

	float flyBaseY = 0.0f;
	float flyTime = 0.0f;
	float flyBobAmp = 10.0f;    
	float flyBobFreq = 3.0f;    
	float flySpeed = 80.0f;    
	float flyAggroRange = 260.0f;

	bool isSlowedDown = false;
	float slowdownTimer = 0.0f;
	const float slowdownDuration = 1.0f; 
	const float slowdownMultiplier = 0.3f; 

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
	float stateTransitionTime = 0.0f;  
	int playerDetectionRadiusTiles = 12;  
	bool bossAwareOfPlayer = false;
	float bossSpeed = 1.5f;  
	int bossDirection = 1;  
	float bossIdleMoveTimer = 0.0f;  
	float bossIdleMoveInterval = 3.0f;  

	PhysBody* attack1Projectile = nullptr;  
	Vector2D attack1ProjectilePos;  
	Vector2D attack1ProjectileDir;  
	float attack1ProjectileSpeed = 200.0f;  
	float attack1ProjectileDistance = 0.0f;  
	const float attack1ProjectileMaxDistance = 128.0f;  
	bool attack1Active = false; 
	
	bool attack1Approaching = false;  
	float attack1WaitTimer = 0.0f;  
	const float attack1WaitDuration = 0.5f;  
	const float attack1Distance = 60.0f;  

	float flyingDuration = 3.0f;  
	float flyingTimer = 0.0f;
	Vector2D bossSpawnPosition; 
	
	float bossCurrentVelocityX = 0.0f;  
	float bossCurrentVelocityY = 0.0f;
	float bossFlightAcceleration = 2.0f; 
	float bossMaxFlightSpeed = 32.0f; 

	BossState lastBossState = BossState::IDLE;  
	float stateUnchangedTimer = 0.0f;  
	const float stateTimeoutDuration = 6.0f; 
};