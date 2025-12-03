#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();
	
	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	// L08 TODO 6: Define OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

private:

	Vector2D spawnPosition;
	void GetPhysicsValues();
	void Move();
	void Jump();
	void Dash();
	void Attack(float dt);
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);

public:

	//Declare player parameters
	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW, texH;

	//Audio fx
	int pickCoinFxId;

	// L08 TODO 5: Add physics to the player - declare a Physics body
	PhysBody* pbody;
	float jumpForce = 2.5f; // The force to apply when jumping
	float dashForce = 50.0f; // The force to apply when dashing
	int jumpCount = 0; // Counter to track the number of jumps
	bool isJumping = false; // Flag to check if the player is currently jumping
	bool isDashing = false; // Flag to check if the player is currently dashing
	bool onGround = false;
	bool canDash = true; // Flag to check if the player can dash
	bool facingLeft = false;
	bool GodMode = false; // Flag for God Mode

	// New: only allow attack after picking Daga
	bool canAttack = false;

private: 
	b2Vec2 velocity;
	AnimationSet anims;

	float currentDashSpeed = 0.0f;    
    float maxDashSpeed = 10.0f;       
    float dashAcceleration = 2.0f;   
    float dashDeceleration = 1.0f;   
    bool isDecelerating = false;    

	// Attack (visual projectile) state
	bool attackActive = false;
	Vector2D attackPos;          // current position in pixels/world coords
	Vector2D attackDir;          // normalized direction (pixels)
	float attackRemaining = 0.0f; // pixels remaining to travel
	const float attackTotal = 16.0f; // total travel distance in pixels
	float attackSpeed = 300.0f;  // pixels per second (tweakable)
	int attackLength = 24;        // visual triangle length in pixels
	int attackHalfWidth = 4;     // half base width in pixels

	// Last look direction used when no WASD input on attack
	Vector2D lookDir;
};