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

	// Guardar jugador
	pugi::xml_node p = root.append_child("player");
	p.append_attribute("x") = player->position.getX();
	p.append_attribute("y") = player->position.getY();

	// Guardar entidades
	pugi::xml_node ents = root.append_child("entities");

	for (auto& e : Engine::GetInstance().entityManager->entities)
	{
		pugi::xml_node n = ents.append_child("entity");
		n.append_attribute("type") = (int)e->type;
		n.append_attribute("x") = e->position.getX();
		n.append_attribute("y") = e->position.getY();
	}

	file.save_file(path.c_str());
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

	// Cargar jugador
	auto p = root.child("player");
	player->position = Vector2D(
		p.attribute("x").as_float(),
		p.attribute("y").as_float()
	);

	// Reset entidades
	Engine::GetInstance().entityManager->entities.clear();

	for (auto e : root.child("entities").children("entity"))
	{
		EntityType t = (EntityType)e.attribute("type").as_int();
		float x = e.attribute("x").as_float();
		float y = e.attribute("y").as_float();

		auto ent = Engine::GetInstance().entityManager->CreateEntity(t);
		if (ent)
			ent->position = Vector2D(x, y);
	}

	// Mostrar notificación en pantalla
	ShowLoadNotification(slot);

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



