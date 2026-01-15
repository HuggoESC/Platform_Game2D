#include <direct.h>
#include "Engine.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene.h"
#include "Log.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Player.h"
#include "Map.h"
#include "Item.h"
#include "Enemy.h"
#include "LifeUP.h"

#include <cstring> // strcmp

static const char* EntityTypeToString(EntityType t)
{
	switch (t)
	{
	case EntityType::PLAYER: return "PLAYER";
	case EntityType::ITEM:   return "ITEM";
	case EntityType::ENEMY:  return "ENEMY";
	case EntityType::LIFEUP: return "LIFEUP";
	default:                 return "UNKNOWN";
	}
}

static EntityType StringToEntityType(const char* s)
{
	if (!s) return EntityType::UNKNOWN;

	if (strcmp(s, "PLAYER") == 0) return EntityType::PLAYER;
	if (strcmp(s, "ITEM") == 0)   return EntityType::ITEM;
	if (strcmp(s, "ENEMY") == 0)  return EntityType::ENEMY;
	if (strcmp(s, "LIFEUP") == 0) return EntityType::LIFEUP;

	return EntityType::UNKNOWN;
}

Scene::Scene() : Module()
{
	name = "scene";
}

// Destructor
Scene::~Scene()
{}

// Called before render is available
bool Scene::Awake()
{
	LOG("Loading Scene");
	bool ret = true;

	// Instantiate the player using the entity manager
	player = std::dynamic_pointer_cast<Player>(
		Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER)
	);

	return ret;
}

// Called before the first frame
bool Scene::Start()
{
	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/level-iv-339695.wav");
	Engine::GetInstance().map->Load("Assets/Maps/", "Level01.tmx");
	pauseTexture = Engine::GetInstance().textures->Load("Assets/Pantallas/PAUSE.png");

	gameState = GameState::PLAYING;

	return true;
}

// Called each loop iteration
bool Scene::PreUpdate()
{
	return true;
}

// Called each loop iteration
bool Scene::Update(float dt)
{
	// Make the camera movement independent of framerate
	float camSpeed = 1;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
	{
		if (gameState == GameState::PLAYING)
			gameState = GameState::PAUSED;
		else if (gameState == GameState::PAUSED)
			gameState = GameState::PLAYING;

		LOG("PAUSE TOGGLE -> %s", (gameState == GameState::PAUSED) ? "PAUSED" : "PLAYING");
	}

	if (gameState == GameState::PAUSED)
		return true;

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y -= (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y += (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x -= (int)ceil(camSpeed * dt);
	
	if(Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		Engine::GetInstance().render.get()->camera.x += (int)ceil(camSpeed * dt);

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
	{
		LOG("Guardado solicitado (slot 1)");
		pendingSave = true;
		pendingSlot = 1;
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
	{
		LOG("Carga solicitada (slot 1)");
		pendingLoad = true;
		pendingSlot = 1;
	}

	return true;
}


bool Scene::SaveGameToSlot(int slot)
{
	// Asegura que existe la carpeta Saves
	_mkdir("saves");

	std::string path = "Saves/slot" + std::to_string(slot) + ".xml";
	LOG("Guardando slot %d...", slot);

	pugi::xml_document file;
	pugi::xml_node root = file.append_child("save");

	// Guardar jugador
	pugi::xml_node p = root.append_child("player");
	p.append_attribute("x") = player->position.getX();
	p.append_attribute("y") = player->position.getY();

	p.append_attribute("hp") = player->hp;
	p.append_attribute("maxHp") = player->maxHp;

	p.append_attribute("spawn_x") = player->spawnPosition.getX();
	p.append_attribute("spawn_y") = player->spawnPosition.getY();

	// Guardar cámara
	pugi::xml_node cam = root.append_child("camera");
	cam.append_attribute("x") = Engine::GetInstance().render->camera.x;
	cam.append_attribute("y") = Engine::GetInstance().render->camera.y;

	// Guardar entidades (NO guardes al player aquí)
	pugi::xml_node ents = root.append_child("entities");

	for (auto& e : Engine::GetInstance().entityManager->entities)
	{
		if (!e) continue;
		if (e->type == EntityType::PLAYER) continue;
		if (e->type == EntityType::HOGUERA) continue;

		pugi::xml_node n = ents.append_child("entity");
		n.append_attribute("type") = EntityTypeToString(e->type);
		n.append_attribute("x") = e->position.getX();
		n.append_attribute("y") = e->position.getY();
	}

	bool ok = file.save_file(path.c_str());
	if (!ok)
	{
		LOG("ERROR: No se pudo guardar el slot %d en %s", slot, path.c_str());
		return false;
	}

	LOG("Slot %d guardado correctamente.", slot);
	return true;
}
bool Scene::LoadGameFromSlot(int slot)
{
	std::string path = "Saves/slot" + std::to_string(slot) + ".xml";
	LOG("Cargando slot %d...", slot);

	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(path.c_str());

	if (!res)
	{
		LOG("ERROR: El slot %d no existe o está vacío.", slot);
		return false;
	}

	Engine::GetInstance().physics->isLoading = true;
	Engine::GetInstance().physics->ignoreContactSteps = 2;

	pugi::xml_node root = file.child("save");

	// 1) Cargar jugador (posición + mover cuerpo físico)
	auto p = root.child("player");
	float px = p.attribute("x").as_float();
	float py = p.attribute("y").as_float();

	player->position = Vector2D(px, py);
	if (player->pbody)
		player->pbody->SetPosition((int)px, (int)py);

	if (p.attribute("hp"))
		player->hp = p.attribute("hp").as_int(4);
	else
		player->hp = 4;

	if (p.attribute("maxHp"))
		player->maxHp = p.attribute("maxHp").as_int(4);
	else
		player->maxHp = 4;

	// Clamp de seguridad
	if (player->maxHp < 4) player->maxHp = 4;
	if (player->maxHp > Player::MAX_HP) player->maxHp = Player::MAX_HP;

	if (player->hp < 0) player->hp = 0;
	if (player->hp > player->maxHp) player->hp = player->maxHp;

	// Cargar checkpoint (si existe)
	if (p.attribute("spawn_x") && p.attribute("spawn_y"))
	{
		player->spawnPosition = Vector2D(
			p.attribute("spawn_x").as_float(),
			p.attribute("spawn_y").as_float()
		);
	}
	else
	{
		// compatibilidad: si no existía en saves antiguos
		player->spawnPosition = player->position;
	}

	// Cargar cámara

	auto cam = root.child("camera");
	if (cam)
	{
		Engine::GetInstance().render->camera.x = cam.attribute("x").as_int();
		Engine::GetInstance().render->camera.y = cam.attribute("y").as_int();
	}

	// 2) Borrar entidades actuales correctamente (evita colliders fantasma)
	auto& list = Engine::GetInstance().entityManager->entities;
	for (auto it = list.begin(); it != list.end(); )
	{
		if (*it &&
			(*it)->type != EntityType::PLAYER &&
			(*it)->type != EntityType::HOGUERA)   // <-- NO BORRAR HOGUERA
		{
			(*it)->CleanUp();
			it = list.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 3) Recrear entidades guardadas, y LLAMAR Start()
	for (auto e : root.child("entities").children("entity"))
	{
		EntityType t = StringToEntityType(e.attribute("type").as_string());
		float x = e.attribute("x").as_float();
		float y = e.attribute("y").as_float();

		auto ent = Engine::GetInstance().entityManager->CreateEntity(t);
		if (ent)
		{
			// Para la mayoría: position + Start
			ent->position = Vector2D(x, y);
			ent->Start();

			// PERO Enemy necesita SetPosition porque su pbody se crea en el constructor
			if (t == EntityType::ENEMY)
			{
				auto slime = std::dynamic_pointer_cast<Enemy>(ent);
				if (slime)
					slime->SetPosition((int)x, (int)y);
			}
		}
	}

	// Mostrar notificación en pantalla
	ShowLoadNotification(slot);

	Engine::GetInstance().physics->isLoading = false;

	return true;
}

void Scene::RequestSave(int slot)
{
	pendingSave = true;
	pendingSlot = slot;
}

void Scene::RequestLoad(int slot)
{
	pendingLoad = true;
	pendingSlot = slot;
}

void Scene::SetCheckpoint(const Vector2D& pos)
{
	player->spawnPosition = pos;
	LOG("Checkpoint actualizado: %.1f %.1f", pos.getX(), pos.getY());

	// Autosave al slot 1 (pero DIFERIDO, seguro con Box2D)
	RequestSave(1);
}

void Scene::ShowLoadNotification(int slot)
{
	loadNotificationSlot = slot;
	loadNotificationTimer = 2.0f; // 2 segundos
}

void Scene::DrawLoadNotification()
{
	if (loadNotificationTimer > 0.0f)
	{
		loadNotificationTimer -= Engine::GetInstance().GetDt();

		SDL_Rect rect = { 20, 20, 180, 30 };

		// Fondo negro (ignorando cámara)
		Engine::GetInstance().render->DrawRectangle(rect, 0, 0, 0, 200, true, false);

		LOG("Cargando slot %d...", loadNotificationSlot);
	}
}

void Scene::TriggerGameOver() 
{
	gameOverActive = true;
	gameOverTimer = 2.0f;
}

void Scene::DrawGameOver()
{
	if (!gameOverActive)
		return;

	float dtSec = Engine::GetInstance().GetDt() / 1000.0f;
	gameOverTimer -= dtSec;

	int w, h;
	Engine::GetInstance().window->GetWindowSize(w, h);

	SDL_Rect rec = { 0, 0, w, h };

	Engine::GetInstance().render->DrawRectangle(rec, 0, 0, 0, 255, true, false);

	if (gameOverTimer <= 0.0f)
	{
		gameOverActive = false;

		// Cargar el último checkpoint guardado (slot 1)
		RequestLoad(1);
	}
}

// Called each loop iteration
bool Scene::PostUpdate()
{
	bool ret = true;

	// Ejecutar Save/Load de forma diferida (evita crasheos con Box2D)
	if (pendingSave)
	{
		pendingSave = false;
		SaveGameToSlot(pendingSlot);
	}

	if (pendingLoad)
	{
		pendingLoad = false;
		LoadGameFromSlot(pendingSlot);
	}

	// Mostrar notificación de carga durante X segundos
	DrawLoadNotification();
	DrawGameOver();

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	if (gameState == GameState::PAUSED)
	{
		// Guardar cámara actual
		int oldCamX = Engine::GetInstance().render->camera.x;
		int oldCamY = Engine::GetInstance().render->camera.y;

		// Forzar cámara a 0 para dibujar UI
		Engine::GetInstance().render->camera.x = 0;
		Engine::GetInstance().render->camera.y = 0;

		// Dibujar la textura a pantalla completa en (0,0)
		Engine::GetInstance().render->DrawTexture(
			pauseTexture,
			0,
			0,
			nullptr,
			1.0f,
			0.0,
			0,
			0,
			false
		);

		// Restaurar cámara
		Engine::GetInstance().render->camera.x = oldCamX;
		Engine::GetInstance().render->camera.y = oldCamY;
	}

	return ret;
}

// Called before quitting
bool Scene::CleanUp()
{

	if (pauseTexture)
	{
		Engine::GetInstance().textures->UnLoad(pauseTexture);
		pauseTexture = nullptr;
	}

	LOG("Freeing scene");

	return true;
}



