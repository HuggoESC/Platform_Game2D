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

	Vector2D spawnPosition;
	
	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();
 
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	void ApplyLifeUp(int amount = 1);
	void ResetLivesAfterGameOver();

	int hp = 4; 
	int maxHp = 4;
	static const int MAX_HP = 12; 

	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW, texH;

	int pickCoinFxId;
	int pickliveFxId;
	int pickGemFxId;	

	int gemsCollected = 0;

	PhysBody* pbody;
	float jumpForce = 2.0f; 
	float dashForce = 100.0f;
	int jumpCount = 0; 
	bool isJumping = false; 
	bool isDashing = false; 
	bool onGround = false;
	bool canDash = true; 
	bool facingLeft = false;
	bool GodMode = false; 

	bool canAttack = false;

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Dash();
	void Attack(float dt);
	void CircularAttack();
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);
	void UpdateLifeAnimation();

	b2Vec2 velocity;
	AnimationSet anims;

	SDL_Texture* lifeTexture = nullptr;
	AnimationSet lifeAnims;
	int lifeTexW = 0;
	int lifeTexH = 0;

	SDL_Texture* daggerUITexture = nullptr;
	int daggerUITexW = 0;
	int daggerUITexH = 0;

	float currentDashSpeed = 0.0f;    
    float maxDashSpeed = 10.0f;       
    float dashAcceleration = 2.0f;   
    float dashDeceleration = 1.0f;   
    bool isDecelerating = false;    

	bool attackActive = false;
	Vector2D attackPos;          
	Vector2D attackDir;          
	float attackRemaining = 0.0f; 
	const float attackTotal = 16.0f; 
	float attackSpeed = 300.0f;  
	int attackLength = 24;       
	int attackHalfWidth = 4;     

	float attackCooldown = 0.0f;
	const float attackCooldownDuration = 1.0f; 

	bool circularAttackActive = false;
	float circularAttackTimer = 0.0f;
	const float circularAttackDuration = 1.0f;     
	const float circularAttackRadius = 32.0f;      
	const float circularAttackCooldown = 11.0f;   
	float circularAttackCooldownTimer = 0.0f;

	bool invulnerable = false;
	float invulnTimer = 0.0f;         
	const float invulnDuration = 0.9f; 

	bool blinkVisible = true;
	float blinkTimer = 0.0f;
	const float blinkInterval = 0.08f;

	Vector2D lookDir;

	std::vector<std::shared_ptr<Entity>> enemiesToDestroy;
};