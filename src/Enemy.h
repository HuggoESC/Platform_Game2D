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
	FLYING    // el nuevo enemigo volador
};

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

	bool CleanUp() override;

	bool flyFacingLeft = true;
	bool Update(float dt) override; // dt en milisegundos
	void OnCollision(PhysBody* physA, PhysBody* physB) override; // dt en milisegundos
	bool Destroy() override;

	void SetPosition(int x, int y);
	void MakeFlying(int frameW = 79, int frameH = 69);

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

};