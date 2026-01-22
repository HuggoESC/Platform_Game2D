#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "Enemy.h"
#include "Window.h"

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {

	position = Vector2D(96,650);
	spawnPosition = position;

	lookDir.setX(1.0f);
	lookDir.setY(0.0f);

	return true;
}

bool Player::Start() {

	std::unordered_map<int, std::string> aliases = { {0,"idle"},{10,"move"},{20,"jump"} };
	anims.LoadFromTSX("Assets/Textures/satiro-Sheet v1.1.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/satiro-Sheet v1.1.png");
	lifeTexture = Engine::GetInstance().textures->Load("Assets/Textures/Vida.png");
	Engine::GetInstance().textures->GetSize(lifeTexture, lifeTexW, lifeTexH);

	std::unordered_map<int, std::string> lifeAliases = {
		{0,"life_4"},
		{1,"life_3"},
		{2,"life_2"},
		{3,"life_1"},
		{4,"life_0"}
	};

	lifeAnims.LoadFromTSX("Assets/Textures/Vida.tsx", lifeAliases);

	hp = 4;
	maxHp = 4;
	UpdateLifeAnimation();

	daggerUITexture = Engine::GetInstance().textures->Load("Assets/Textures/Daga.png");
	Engine::GetInstance().textures->GetSize(daggerUITexture, daggerUITexW, daggerUITexH);

	texW =32;
	texH =32;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW /2, bodyType::DYNAMIC);

	pbody->listener = shared_from_this();
	pbody->ctype = ColliderType::PLAYER;

	spawnPosition = position;

	pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");
	pickGemFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/retro-coin-1-236677.wav");
	pickliveFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/life_pickup.wav");

	return true;
}

bool Player::Update(float dt)
{ 
	GetPhysicsValues();
	Move();
	Dash();
	Jump();
	Teleport();
	ApplyPhysics();
	Attack(dt);
	Draw(dt);

	float dtSec = dt / 1000.0f;

	if (invulnerable)
	{
		invulnTimer -= dtSec;
		if (invulnTimer <= 0.0f)
		{
			invulnerable = false;
			blinkVisible = true;
		}

		blinkTimer -= dtSec;
		if (blinkTimer <= 0.0f)
		{
			blinkTimer = blinkInterval;
			blinkVisible = !blinkVisible;
		}
	}

	if (attackCooldown > 0.0f)
	{
		attackCooldown -= dtSec;
	}

	CircularAttack();

	if (circularAttackCooldownTimer > 0.0f)
	{
		circularAttackCooldownTimer -= dtSec;
	}

	if (circularAttackActive)
	{
		circularAttackTimer -= dtSec;
		if (circularAttackTimer <= 0.0f)
		{
			circularAttackActive = false;
		}
	}

	if (!GodMode)
	{
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		float deathY = mapSize.getY() +100.0f;

		if (position.getY() > deathY)
		{
			LOG("Player died. Respawning...");

			if (!invulnerable)
			{
				invulnerable = true;
				invulnTimer = invulnDuration;
				blinkTimer = blinkInterval;
				blinkVisible = true;

				hp -= 1;
				if (hp < 0) hp = 0;
				UpdateLifeAnimation();

				if (hp <= 0)
				{
					Engine::GetInstance().scene->TriggerGameOver();
					return true;
				}
			}

			Engine::GetInstance().physics->SetLinearVelocity(pbody, {0.0f,0.0f });

			pbody->SetPosition((int)spawnPosition.getX(), (int)spawnPosition.getY());

			position = spawnPosition;

			Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w /14;
			Engine::GetInstance().render->camera.y = -position.getY() + Engine::GetInstance().render->camera.h *9 /10;

			anims.SetCurrent("idle");
			isJumping = false;
			isDashing = false;
			canDash = true;
			jumpCount =0;
		}
	}
	
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
		GodMode = !GodMode;
		LOG("GOD MODE: %S", GodMode ? "ON" : "OFF");
	}
	
	return true;
}

void Player::Teleport() {
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) {
		pbody->SetPosition(96,96);
	}
}

void Player::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = {0, velocity.y };
}

void Player::Move() {
	if (isDashing && !isDecelerating) return;

	int in_dx =0, in_dy =0;
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) in_dy -=1;
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) in_dy +=1;
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) in_dx -=1;
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) in_dx +=1;
	if (in_dx !=0 || in_dy !=0) {
		float fx = (float)in_dx;
		float fy = (float)in_dy;
		float len = sqrtf(fx * fx + fy * fy);
		if (len !=0.0f) {
			lookDir.setX(fx / len);
			lookDir.setY(fy / len);
		}
	}

	if (GodMode)
	{
		float gmSpeed =6.0f;

		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
			pbody->SetPosition(position.getX(), position.getY() - gmSpeed);
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
			pbody->SetPosition(position.getX(), position.getY() + gmSpeed);
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			pbody->SetPosition(position.getX() - gmSpeed, position.getY());
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
			pbody->SetPosition(position.getX() + gmSpeed, position.getY());

		return; 
	}
 
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -speed;
		anims.SetCurrent("move");
		facingLeft = true;
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = speed;
		anims.SetCurrent("move");
		facingLeft = false;
	}
	else {
		anims.SetCurrent("idle");
	}
}

void Player::Jump()
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		if (jumpCount ==0)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody,0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody,0.0f, -jumpForce, true);

			isJumping = true;
			onGround = false;
			jumpCount =1;
			anims.SetCurrent("jump");
		}
		else if (jumpCount ==1)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody,0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody,0.0f, -jumpForce, true);

			isJumping = true;
			jumpCount =2;
			anims.SetCurrent("jump");
		}
	}
}

void Player::Dash() {
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN && !isDashing && canDash == true) {
		isDashing = true;
		canDash = false;
		isDecelerating = false;
		currentDashSpeed =5.0f; 
	}
}

void Player::ApplyPhysics() {
	if (isDashing) {
		if (!isDecelerating) {
			currentDashSpeed += dashAcceleration;
			
			if (currentDashSpeed >= maxDashSpeed) {
				isDecelerating = true;
				currentDashSpeed = maxDashSpeed;
			}
		}
		else {
			currentDashSpeed -= dashDeceleration;
			
			if (currentDashSpeed <=0.0f) {
				isDashing = false;
				isDecelerating = false;
				currentDashSpeed =0.0f;
			}
		}

		float direction = facingLeft ? -1.0f :1.0f;
		velocity.x = currentDashSpeed * direction;
		velocity.y =0.0f;
	}

	else if (isJumping) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Attack(float dt) {
	if (attackActive) {
		float move = attackSpeed * (dt /1000.0f);
		if (move > attackRemaining) move = attackRemaining;
		attackRemaining -= move;
		attackPos.setX(attackPos.getX() + attackDir.getX() * move);
		attackPos.setY(attackPos.getY() + attackDir.getY() * move);

		LOG("Attack active at (%.1f, %.1f), remaining: %.1f", attackPos.getX(), attackPos.getY(), attackRemaining);

		float attackHitRange = 20.0f;
		for (const auto& entity : Engine::GetInstance().entityManager->entities) {
			if (entity->type == EntityType::ENEMY) {
				Enemy* enemy = dynamic_cast<Enemy*>(entity.get());
				if (enemy) {
					float ex = enemy->position.getX();
					float ey = enemy->position.getY();
					float dx = attackPos.getX() - ex;
					float dy = attackPos.getY() - ey;
					float dist = std::sqrt(dx * dx + dy * dy);
					LOG("Enemy at (%.1f, %.1f), distance: %.1f", ex, ey, dist);
					
					if (enemy->IsBoss())
					{
						if (dist <= 100.0f)
						{
							LOG("Boss hit! Reducing health...");
							enemy->OnAttackHit();
						}
					}
					else if (dist <= 20.0f && enemy->IsHitByAttack(attackPos.getX(), attackPos.getY(), attackHitRange))
					{
						LOG("ENEMY HIT!");
						enemiesToDestroy.push_back(entity);
					}
				}
			}
		}

		for (const auto& enemy : enemiesToDestroy) {
			enemy->Destroy();
		}
		enemiesToDestroy.clear();

		if (attackRemaining <=0.0f) {
			attackActive = false;
		}
		return;
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
		if (!canAttack) {
			LOG("Cannot attack - no Daga picked up");
			return;
		}

		if (attackCooldown > 0.0f) {
			LOG("Attack on cooldown: %.2f seconds remaining", attackCooldown);
			return;
		}

		LOG("Attack started!");

		int dx =0, dy =0;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) dy -=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) dy +=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) dx -=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) dx +=1;

		float fx =0.0f, fy =0.0f;

		if (dx ==0 && dy ==0) {
			fx = lookDir.getX();
			fy = lookDir.getY();
			if (fx ==0.0f && fy ==0.0f) { fx =1.0f; fy =0.0f; }
		}
		else {
			fx = (float)dx;
			fy = (float)dy;
			float len = sqrtf(fx * fx + fy * fy);
			if (len ==0.0f) return;
			fx /= len;
			fy /= len;
			lookDir.setX(fx);
			lookDir.setY(fy);
		}

		int px =0, py =0;
		if (pbody) pbody->GetPosition(px, py);

		const float spawnOffset = (float)(texW /2) +12.0f;

		attackDir.setX(fx);
		attackDir.setY(fy);

		attackPos.setX((float)px + attackDir.getX() * spawnOffset);
		attackPos.setY((float)py + attackDir.getY() * spawnOffset);

		attackRemaining = attackTotal;
		attackActive = true;
		attackCooldown = attackCooldownDuration;

		float initialMove = std::min(2.0f, attackRemaining);
		attackPos.setX(attackPos.getX() + attackDir.getX() * initialMove);
		attackPos.setY(attackPos.getY() + attackDir.getY() * initialMove);
		attackRemaining -= initialMove;
		if (attackRemaining <=0.0f) attackActive = false;
	}
}

void Player::CircularAttack() {
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN) {
		if (circularAttackCooldownTimer <= 0.0f) {
			LOG("Circular Attack activated!");
			circularAttackActive = true;
			circularAttackTimer = circularAttackDuration;
			circularAttackCooldownTimer = circularAttackCooldown;
		} else {
			LOG("Circular Attack on cooldown: %.2f seconds remaining", circularAttackCooldownTimer);
		}
	}

	if (circularAttackActive) {
		int px = 0, py = 0;
		if (pbody) pbody->GetPosition(px, py);

		for (const auto& entity : Engine::GetInstance().entityManager->entities) {
			if (entity->type == EntityType::ENEMY) {
				Enemy* enemy = dynamic_cast<Enemy*>(entity.get());
				if (enemy) {
					float ex = enemy->position.getX();
					float ey = enemy->position.getY();
					float dx = (float)px - ex;
					float dy = (float)py - ey;
					float dist = std::sqrt(dx * dx + dy * dy);

					if (dist <= circularAttackRadius) {
						LOG("Circular Attack hit enemy at distance %.1f", dist);
						
						if (enemy->IsBoss()) {
							LOG("Circular Attack cannot damage BOSS!");
						} else {
							LOG("Regular enemy hit by circular attack!");
							enemiesToDestroy.push_back(entity);
						}
					}
				}
			}
		}

		for (const auto& enemy : enemiesToDestroy) {
			enemy->Destroy();
		}
		enemiesToDestroy.clear();
	}
}

static const char* LifeFrameNameFromQuarters(int q)
{
	if (q <= 0) return "life_0";
	if (q == 1) return "life_1";
	if (q == 2) return "life_2";
	if (q == 3) return "life_3";
	return "life_4";
}

void Player::Draw(float dt) {

	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float limitLeft = Engine::GetInstance().render->camera.w /4;
	float limitRight = mapSize.getX() - Engine::GetInstance().render->camera.w *3 /4;
	
	if (position.getX() - limitLeft >0 && position.getX() < limitRight) {
		Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w /4;
	}

	SDL_Rect frame = animFrame;

	if (!(invulnerable && !blinkVisible))
	{
		Engine::GetInstance().render->DrawTexture(
			texture,
			x - texW / 2,
			y - texH / 2,
			&animFrame,
			1.0f,
			0.0,
			0,
			0,
			facingLeft
		);
	}

	if (attackActive) {
		float tx = attackPos.getX();
		float ty = attackPos.getY();

		float dx = attackDir.getX();
		float dy = attackDir.getY();

		float bx = tx - dx * (float)attackLength;
		float by = ty - dy * (float)attackLength;

		float px = -dy;
		float py = dx;
		float hw = (float)attackHalfWidth;

		int ax = (int)std::round(bx + px * hw);
		int ay = (int)std::round(by + py * hw);
		int bx_i = (int)std::round(bx - px * hw);
		int by_i = (int)std::round(by - py * hw);
		int tipx = (int)std::round(tx);
		int tipy = (int)std::round(ty);

		Engine::GetInstance().render->DrawLine(tipx, tipy, ax, ay,255,255,255,255, true);
		Engine::GetInstance().render->DrawLine(ax, ay, bx_i, by_i,255,255,255,255, true);
		Engine::GetInstance().render->DrawLine(bx_i, by_i, tipx, tipy,255,255,255,255, true);
	}

	if (circularAttackActive) {
		Engine::GetInstance().render->DrawCircle(x, y, (int)circularAttackRadius, 0, 255, 0, 200);
	}

	if (lifeTexture)
	{
		const int hudScreenX = 20;
		const int hudScreenY = 20;

		int scale = Engine::GetInstance().window->GetScale();
		int camX = Engine::GetInstance().render->camera.x;
		int camY = Engine::GetInstance().render->camera.y;

		int baseX = (hudScreenX - camX) / scale;
		int baseY = (hudScreenY - camY) / scale;

		const int heartsToDraw = 3;
		const int spacing = 34;

		for (int i = 0; i < heartsToDraw; ++i)
		{
			int q = hp - i * 4;
			if (q < 0) q = 0;
			if (q > 4) q = 4;

			const char* animName = LifeFrameNameFromQuarters(q);
			if (lifeAnims.Has(animName))
				lifeAnims.SetCurrent(animName);

			SDL_Rect frame = lifeAnims.GetCurrentFrame();

			Engine::GetInstance().render->DrawTexture(
				lifeTexture,
				baseX + i * spacing,
				baseY,
				&frame
			);
		}

		if (canAttack && daggerUITexture)
		{
			int daggerX = baseX + heartsToDraw * spacing + 16;
			int daggerY = baseY + 16;

			Engine::GetInstance().render->DrawTexture(
				daggerUITexture,
				daggerX - daggerUITexW / 2,
				daggerY - daggerUITexH / 2
			);

			if (attackCooldown > 0.0f)
			{
				float cooldownProgress = attackCooldown / attackCooldownDuration * -1;
				int cooldownHeight = (int)(daggerUITexH * cooldownProgress);

				SDL_Rect cooldownRect = {
					(int)(daggerX - daggerUITexW / 2),
					(int)(daggerY - daggerUITexH / 2) + 32,
					daggerUITexW,
					cooldownHeight
				};

				Engine::GetInstance().render->DrawRectangle(
					cooldownRect,
					255, 255, 255, 255,
					true,
					true
				);
			}
		}
	}

}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {

	if (GodMode) return;

	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
	case ColliderType::TOPE:
		LOG("Collision GROUND (PLATFORM/TOPE)");
		onGround = true;
		isJumping = false;
		isDashing = false;
		canDash = true;
		jumpCount =0;
		anims.SetCurrent("idle");
		break;

	case ColliderType::ITEM:
	{
		LOG("Collision ITEM");
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);

		auto pickup = physB ? physB->listener.lock() : std::shared_ptr<Entity>();
		if (physB) physB->listener.reset();

		if (pickup && pickup->active)
		{
			pickup->Destroy();
		}

		canAttack = true;
		break;
	}

	case ColliderType::GEM:
	{
		LOG("Collision GEM");
		Engine::GetInstance().audio->PlayFx(pickGemFxId);

		auto pickup = physB ? physB->listener.lock() : std::shared_ptr<Entity>();
		if (physB) physB->listener.reset();

		if (pickup && pickup->active)
		{
			pickup->Destroy();
			gemsCollected++;
		}
		break;
	}

	case ColliderType::LIFEUP:
	{
		auto pickup = physB ? physB->listener.lock() : std::shared_ptr<Entity>();
		if (physB) physB->listener.reset();

		if (!pickup || !pickup->active) break;

		Engine::GetInstance().audio->PlayFx(pickliveFxId);
		ApplyLifeUp();

		pickup->Destroy();

		break;
	}

	case ColliderType::ENEMY:
	{
		if (invulnerable) break;

		LOG("Collision ENEMY - Damage!");

		invulnerable = true;
		invulnTimer = invulnDuration;
		blinkTimer = blinkInterval;
		blinkVisible = true;

		hp -= 1;
		if (hp < 0) hp = 0;
		UpdateLifeAnimation();

		float kx = facingLeft ? 2.5f : -2.5f;
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, kx, -0.2f, true);

		if (hp <= 0)
		{
			Engine::GetInstance().scene->TriggerGameOver();
		}

		break;
	}

	case ColliderType::UNKNOWN:
	{
		if (invulnerable) break;

		LOG("Collision BOSS PROJECTILE - Damage!");

		invulnerable = true;
		invulnTimer = invulnDuration;
		blinkTimer = blinkInterval;
		blinkVisible = true;

		hp -= 1;
		if (hp < 0) hp = 0;
		UpdateLifeAnimation();

		float kx = facingLeft ? 2.5f : -2.5f;
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, kx, -0.2f, true);

		if (hp <= 0)
		{
			Engine::GetInstance().scene->TriggerGameOver();
		}

		break;
	}

	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (GodMode) return;

	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
	case ColliderType::TOPE:
		LOG("End Collision GROUND (PLATFORM/TOPE)");
		onGround = false;
		break;

	case ColliderType::ITEM:
		LOG("End Collision ITEM");
		break;

	case ColliderType::UNKNOWN:
		LOG("End Collision UNKNOWN");
		break;

	default:
		break;
	}
}

void Player::UpdateLifeAnimation()
{
	if (maxHp < 4) maxHp = 4;
	if (maxHp > MAX_HP) maxHp = MAX_HP;

	if (hp < 0) hp = 0;
	if (hp > maxHp) hp = maxHp;
}

void Player::ApplyLifeUp(int amount)
{
	if (amount <= 0) return;

	if (hp < maxHp)
	{
		hp += amount;
		if (hp > maxHp) hp = maxHp;
		UpdateLifeAnimation();
		return;
	}

	if (maxHp < MAX_HP)
	{
		maxHp += amount;
		if (maxHp > MAX_HP) maxHp = MAX_HP;

		hp += amount;
		if (hp > maxHp) hp = maxHp;

		UpdateLifeAnimation();
	}
}

void Player::ResetLivesAfterGameOver()
{
	if (maxHp < 4) maxHp = 4;

	hp = 4;
	if (hp > maxHp) hp = maxHp;

	UpdateLifeAnimation();
}

