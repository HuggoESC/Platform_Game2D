#pragma once

#include "Entity.h"
#include "Animation.h"
#include "SDL3/SDL.h"
#include <box2d/box2d.h>
#include "Physics.h" 
#include <random>

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

	bool CleanUp() override;

	bool Update(float dt) override; // dt en milisegundos
	void OnCollision(PhysBody* physA, PhysBody* physB) override; // dt en milisegundos
	bool Destroy() override;

	void SetPosition(int x, int y)
	{
		position.setX(x);
		position.setY(y);

		if (pbody != nullptr)
			pbody->SetPosition((float)x, (float)y);
	}

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
};