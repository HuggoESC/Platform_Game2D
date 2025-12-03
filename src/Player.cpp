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

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {

	//L03: TODO 2: Initialize Player parameters
	position = Vector2D(96,650);
	spawnPosition = position;
	return true;
}

bool Player::Start() {

	// load
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{10,"move"},{20,"jump"} };
	anims.LoadFromTSX("Assets/Textures/satiro-Sheet v1.1.tsx", aliases);
	anims.SetCurrent("idle");

	//L03: TODO 2: Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/satiro-Sheet v1.1.png");

	// L08 TODO 5: Add physics to the player - initialize physics body
	//Engine::GetInstance().textures->GetSize(texture, texW, texH);
	texW = 32;
	texH = 32;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);

	// L08 TODO 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08 TODO 7: Assign collider type
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
		Draw(dt);

	//Muerte por caída (no afecta en GodMode)
	if (!GodMode)
	{
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		// Si cae muy por debajo del mapa = muerto
		float deathY = mapSize.getY() + 100.0f;

		if (position.getY() > deathY)
		{
				LOG("Player died. Respawning...");

				// Reset velocidad física
				Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });

				// Teletransportar al spawn
				pbody->SetPosition((int)spawnPosition.getX(), (int)spawnPosition.getY());

				// Actualizar valores internos de posición
				position = spawnPosition;

				// Reposicionar cámara para centrar al jugador tras el respawn
				Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w / 14;
				Engine::GetInstance().render->camera.y = -position.getY() + Engine::GetInstance().render->camera.h * 9 / 10;

				// Reset estado
				anims.SetCurrent("idle");
				isJumping = false;
				isDashing = false;
				canDash = true;
				jumpCount = 0;
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
		pbody->SetPosition(96, 96);
	}
}

void Player::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
}

void Player::Move() {
    if (isDashing && !isDecelerating) return;

	if (GodMode)
	{
		float gmSpeed = 6.0f;

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
		// ?? PRIMER SALTO: estamos en el suelo
		if (jumpCount == 0)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody, 0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);

			isJumping = true;
			onGround = false;
			jumpCount = 1;        // hemos gastado el primer salto
			anims.SetCurrent("jump");
		}
		// ?? DOBLE SALTO: ya hemos saltado una vez, estamos en el aire
		else if (jumpCount == 1)
		{
			Engine::GetInstance().physics->SetYVelocity(pbody, 0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);

			isJumping = true;
			jumpCount = 2;        // segundo salto gastado
			anims.SetCurrent("jump");
		}
	}
}

void Player::Dash() {
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN && !isDashing && canDash == true) {
        isDashing = true;
		canDash = false;
        isDecelerating = false;
        currentDashSpeed = 5.0f; 
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
            
            if (currentDashSpeed <= 0.0f) {
                isDashing = false;
                isDecelerating = false;
                currentDashSpeed = 0.0f;
            }
        }

        float direction = facingLeft ? -1.0f : 1.0f;
        velocity.x = currentDashSpeed * direction;
        velocity.y = 0.0f;
    }

    else if (isJumping) {
        velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
    }

    // Apply velocity via helper
    Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt) {

	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float limitLeft = Engine::GetInstance().render->camera.w / 4;
	float limitRight = mapSize.getX() - Engine::GetInstance().render->camera.w * 3 / 4;
	if (position.getX() - limitLeft > 0 && position.getX() < limitRight) {
		Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w / 4;
	}

	SDL_Rect frame = animFrame;

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

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

// L08 TODO 6: Define OnCollision function for the player. 
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
		jumpCount = 0;
		anims.SetCurrent("idle");
		break;

	case ColliderType::ITEM:
		LOG("Collision ITEM");
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		if (physB->listener)
			physB->listener->Destroy();
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
		onGround = false;   // ya no estamos apoyados
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

