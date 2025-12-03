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

	//L04: TODO 3b: Instantiate the player using the entity manager
	player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
	
	//L08: TODO 4: Create a new item using the entity manager and set the position to (200, 672) to test
	std::shared_ptr<Item> item = std::dynamic_pointer_cast<Item>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ITEM));
	item->position = Vector2D(640, 480);

	return ret;
}

// Called before the first frame
bool Scene::Start()
{
	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/level-iv-339695.wav");
	Engine::GetInstance().map->Load("Assets/Maps/", "Level01.tmx");
	
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
	//L03 TODO 3: Make the camera movement independent of framerate
	float camSpeed = 1;

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
		LOG("¿Qué slot quieres guardar? (1, 2 o 3)");
		// De momento guardamos siempre slot 1
		SaveGameToSlot(1);
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
	{
		LOG("¿Qué slot quieres cargar? (1, 2 o 3)");
		// De momento cargamos siempre slot 1
		LoadGameFromSlot(1);
	}

	return true;
}


bool Scene::SaveGameToSlot(int slot)
{
	std::string path = "Saves/slot" + std::to_string(slot) + ".xml";
	LOG("Guardando slot %d...", slot);

	pugi::xml_document file;
	pugi::xml_node root = file.append_child("save");

	// Guardar SOLO la posición del jugador
	pugi::xml_node p = root.append_child("player");
	p.append_attribute("x") = player->position.getX();
	p.append_attribute("y") = player->position.getY();

	if (!file.save_file(path.c_str()))
	{
		LOG("ERROR guardando slot %d", slot);
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

	pugi::xml_node root = file.child("save");
	pugi::xml_node p = root.child("player");

	float x = p.attribute("x").as_float();
	float y = p.attribute("y").as_float();

	// Mover la posición lógica del jugador
	player->position = Vector2D(x, y);

	// Mover también el cuerpo físico si existe
	if (player->pbody != nullptr)
		player->pbody->SetPosition((int)x, (int)y);

	ShowLoadNotification(slot);

	LOG("Slot %d cargado correctamente.", slot);
	return true;
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

// Called each loop iteration
bool Scene::PostUpdate()
{
	bool ret = true;

	// Mostrar notificación de carga durante X segundos
	DrawLoadNotification();

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	return ret;
}

// Called before quitting
bool Scene::CleanUp()
{
	LOG("Freeing scene");

	return true;
}



