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

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {

	//L03: TODO2: Initialize Player parameters
	position = Vector2D(96,650);
	spawnPosition = position;

	// default look direction to the right
	lookDir.setX(1.0f);
	lookDir.setY(0.0f);

	return true;
}

bool Player::Start() {

	// load
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{10,"move"},{20,"jump"} };
	anims.LoadFromTSX("Assets/Textures/satiro-Sheet v1.1.tsx", aliases);
	anims.SetCurrent("idle");

	//L03: TODO2: Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/satiro-Sheet v1.1.png");

	// L08 TODO5: Add physics to the player - initialize physics body
	//Engine::GetInstance().textures->GetSize(texture, texW, texH);
	texW =32;
	texH =32;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW /2, bodyType::DYNAMIC);

	// L08 TODO6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08 TODO7: Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	//initialize audio effect
	pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");

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

	//Muerte por ca�da (no afecta en GodMode)
	if (!GodMode)
	{
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		// Si cae muy por debajo del mapa = muerto
		float deathY = mapSize.getY() +100.0f;

		if (position.getY() > deathY)
		{
				LOG("Player died. Respawning...");

				// Reset velocidad f�sica
				Engine::GetInstance().physics->SetLinearVelocity(pbody, {0.0f,0.0f });

				// Teletransportar al spawn
				pbody->SetPosition((int)spawnPosition.getX(), (int)spawnPosition.getY());

				// Actualizar valores internos de posici�n
				position = spawnPosition;

				// Reposicionar c�mara para centrar al jugador tras el respawn
				Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w /14;
				Engine::GetInstance().render->camera.y = -position.getY() + Engine::GetInstance().render->camera.h *9 /10;

				// Reset estado
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
	// Teleport the player to a specific position for testing purposes
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) {
		pbody->SetPosition(96,96);
	}
}

void Player::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = {0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
}

void Player::Move() {
 if (isDashing && !isDecelerating) return;

	// Update look direction from WASD input (if any)
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
 
 // Move left/right
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
	// Solo reaccionamos en el frame donde se pulsa la tecla
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		// PRIMER SALTO: estamos en el suelo
		if (jumpCount ==0)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody,0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody,0.0f, -jumpForce, true);

			isJumping = true;
			onGround = false;
			jumpCount =1; // hemos gastado el primer salto
			anims.SetCurrent("jump");
		}
		// DOBLE SALTO: ya hemos saltado una vez, estamos en el aire
		else if (jumpCount ==1)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody,0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody,0.0f, -jumpForce, true);

			isJumping = true;
			jumpCount =2; // segundo salto gastado
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

 // Apply velocity via helper
 Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}
void Player::Attack(float dt) {
	// dt is in milliseconds
	// If attack already active -> advance it
	if (attackActive) {
		// compute movement this frame (pixels)
		float move = attackSpeed * (dt /1000.0f);
		if (move > attackRemaining) move = attackRemaining;
		attackRemaining -= move;
		attackPos.setX(attackPos.getX() + attackDir.getX() * move);
		attackPos.setY(attackPos.getY() + attackDir.getY() * move);

		LOG("Attack active at (%.1f, %.1f), remaining: %.1f", attackPos.getX(), attackPos.getY(), attackRemaining);

		// Check for enemy hits during attack
		float attackHitRange = 20.0f; // pixels to check around attack position
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
					
					if (enemy->IsHitByAttack(attackPos.getX(), attackPos.getY(), attackHitRange)) {
						LOG("ENEMY HIT! Destroying enemy!");
						// Defer destruction until after the loop completes
						enemiesToDestroy.push_back(entity);
					}
				}
			}
		}

		// After checking all enemies, destroy the ones that were hit
		for (const auto& enemy : enemiesToDestroy) {
			enemy->Destroy();
		}
		enemiesToDestroy.clear();

		if (attackRemaining <=0.0f) {
			attackActive = false;
		}
		return;
	}

	// Start attack when E pressed
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
		// Only allow attacks after picking up the Daga
		if (!canAttack) {
			LOG("Cannot attack - no Daga picked up");
			return;
		}

		LOG("Attack started!");

		// Determine direction from WASD (use current input, allow diagonals)
		int dx =0, dy =0;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) dy -=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) dy +=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) dx -=1;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) dx +=1;

		float fx =0.0f, fy =0.0f;

		// If no WASD input, use lookDir
		if (dx ==0 && dy ==0) {
			fx = lookDir.getX();
			fy = lookDir.getY();
			// fallback to face right if lookDir zero
			if (fx ==0.0f && fy ==0.0f) { fx =1.0f; fy =0.0f; }
		}
		else {
			// Normalize direction
			fx = (float)dx;
			fy = (float)dy;
			float len = sqrtf(fx * fx + fy * fy);
			if (len ==0.0f) return;
			fx /= len;
			fy /= len;
			// update lookDir to this explicit input
			lookDir.setX(fx);
			lookDir.setY(fy);
		}

		// Get player center
		int px =0, py =0;
		if (pbody) pbody->GetPosition(px, py);

		// Spawn the attack *in front* of the player:
		// offset by player's half-width (texW/2) plus a small margin so it doesn't overlap the sprite.
		const float spawnOffset = (float)(texW /2) +12.0f;

		attackDir.setX(fx);
		attackDir.setY(fy);

		attackPos.setX((float)px + attackDir.getX() * spawnOffset);
		attackPos.setY((float)py + attackDir.getY() * spawnOffset);

		attackRemaining = attackTotal;
		attackActive = true;

		// Small immediate advance so triangle is clearly visible (optional)
		float initialMove = std::min(2.0f, attackRemaining);
		attackPos.setX(attackPos.getX() + attackDir.getX() * initialMove);
		attackPos.setY(attackPos.getY() + attackDir.getY() * initialMove);
		attackRemaining -= initialMove;
		if (attackRemaining <=0.0f) attackActive = false;
	}
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

	Engine::GetInstance().render->DrawTexture(
		texture,
		x - texW /2,
		y - texH /2,
		&animFrame,
		1.0f,
		0.0,
		0,
		0,
		facingLeft
	);

	// Draw attack triangle (white) if active
	if (attackActive) {
		// tip at attackPos, base behind tip by attackLength
		float tx = attackPos.getX();
		float ty = attackPos.getY();

		// direction normalized already in attackDir
		float dx = attackDir.getX();
		float dy = attackDir.getY();

		// base center
		float bx = tx - dx * (float)attackLength;
		float by = ty - dy * (float)attackLength;

		// perpendicular
		float px = -dy;
		float py = dx;
		// half width
		float hw = (float)attackHalfWidth;

		// base vertices
		int ax = (int)std::round(bx + px * hw);
		int ay = (int)std::round(by + py * hw);
		int bx_i = (int)std::round(bx - px * hw);
		int by_i = (int)std::round(by - py * hw);
		int tipx = (int)std::round(tx);
		int tipy = (int)std::round(ty);

		// Draw triangle outline (3 lines) in world coords (Render will apply camera)
		Engine::GetInstance().render->DrawLine(tipx, tipy, ax, ay,255,255,255,255, true);
		Engine::GetInstance().render->DrawLine(ax, ay, bx_i, by_i,255,255,255,255, true);
		Engine::GetInstance().render->DrawLine(bx_i, by_i, tipx, tipy,255,255,255,255, true);
	}
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

// L08 TODO6: Define OnCollision function for the player. 
void Player::OnCollision(PhysBody* physA, PhysBody* physB) {

	if (GodMode) return;

	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
	case ColliderType::TOPE:
		LOG("Collision GROUND (PLATFORM/TOPE)");
		// Estamos TOCANDO SUELO
		onGround = true;
		isJumping = false;
		isDashing = false;
		canDash = true;
		jumpCount =0;
		anims.SetCurrent("idle");
		break;

	case ColliderType::ITEM:
		LOG("Collision ITEM");
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		if (physB->listener)
			physB->listener->Destroy();
		// enable attack when picking the dagger
		canAttack = true;
		break;

	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;

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
		onGround = false; // ya no estamos apoyados
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

