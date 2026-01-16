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

static const char* MUSIC_LEVEL1 = "Assets/Audio/Music/level-iv-339695.wav";
static const char* MUSIC_PAUSE = "Assets/Audio/Music/PauseTheme.wav";
static const char* MUSIC_GAMEOVER = "Assets/Audio/Music/GameOver.wav";
static const char* MUSIC_INTRO = "Assets/Audio/Music/IntroTheme.wav";


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
	Engine::GetInstance().map->Load("Assets/Maps/", "Level01.tmx");
	pauseTexture = Engine::GetInstance().textures->Load("Assets/Pantallas/PAUSE.jpeg");
	gameOverTexture = Engine::GetInstance().textures->Load("Assets/Pantallas/GAMEOVER.jpeg");
	introTexture = Engine::GetInstance().textures->Load("Assets/Pantallas/INTRO.jpeg");

	if (gameOverTexture == nullptr)
	{
		LOG("ERROR: No se pudo cargar Assets/Pantallas/GAMEOVER.jpeg");
	}

	if (introTexture == nullptr)
	{
		LOG("ERROR: No se pudo cargar Assets/Pantallas/INTRO.jpeg");
	}

	gameState = GameState::INTRO;
	lastState = (gameState == GameState::INTRO) ? GameState::PLAYING : GameState::INTRO;

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

	// PAUSA con ESC (oficial)
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
	{
		if (gameState == GameState::PLAYING)
		{
			gameState = GameState::PAUSED;
		}
		else if (gameState == GameState::PAUSED)
		{
			gameState = GameState::PLAYING;
		}
		else if (gameState == GameState::LEVELSELECTOR)
		{
			Engine::GetInstance().audio->PlayMusic(MUSIC_LEVEL1); // temporal hasta tener musica propia
		}
	}

	// P como toggle de pausa SOLO para debug (opcional)
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
	{
		LOG("DEBUG: Toggle pause con P");
		gameState = (gameState == GameState::PAUSED) ? GameState::PLAYING : GameState::PAUSED;
	}

	// --- AUDIO por cambio de estado (solo cuando cambia) ---
	if (gameState != lastState)
	{
		if (gameState == GameState::INTRO)
		{
			Engine::GetInstance().audio->PlayMusic(MUSIC_INTRO);
		}
		else if (gameState == GameState::PAUSED)
		{
			Engine::GetInstance().audio->PlayMusic(MUSIC_PAUSE);
		}
		else if (gameState == GameState::GAMEOVER)
		{
			Engine::GetInstance().audio->PlayMusic(MUSIC_GAMEOVER);
		}
		else if (gameState == GameState::LEVELSELECTOR)
		{
			// De momento: usa música del nivel (o crea MUSIC_MENU si quieres)
			Engine::GetInstance().audio->PlayMusic(MUSIC_LEVEL1);
		}
		else if (gameState == GameState::PLAYING)
		{
			Engine::GetInstance().audio->PlayMusic(MUSIC_LEVEL1);
		}

		lastState = gameState;
	}

	// --- Estado INTRO (con botones START/EXIT) ---
	bool introWantsBlock = false;
	if (gameState == GameState::INTRO)
	{
		int mx, my;
		Engine::GetInstance().input->GetMousePosition(mx, my);

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		// Botones centrados (ajústalo si quieres un pelín más arriba/abajo)
		const int btnW = 360;
		const int btnH = 75;
		const int gap = 18;

		int startX = (winW - btnW) / 2;
		int startY_Start = (winH / 2) + 140; // START más arriba
		int startY_Exit = (winH / 2) + 160 + btnH + gap;

		SDL_Rect startRect = { startX, startY_Start, btnW, btnH };
		SDL_Rect exitRect = { startX, startY_Exit,  btnW, btnH };

		auto inside = [&](const SDL_Rect& r) {
			return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
			};

		// Teclado: alterna entre START y EXIT
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_DOWN ||
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_DOWN)
		{
			introOption = (introOption == IntroOption::START) ? IntroOption::EXIT : IntroOption::START;
		}

		// Mouse hover
		if (inside(startRect)) introOption = IntroOption::START;
		if (inside(exitRect))  introOption = IntroOption::EXIT;

		bool activate =
			(Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN) ||
			(Engine::GetInstance().input->GetMouseButtonDown(1) == KEY_DOWN);

		if (activate)
		{
			if (introOption == IntroOption::START)
			{
				// Lo mismo que "Enter": avanzar
				gameState = GameState::LEVELSELECTOR;  // o PLAYING si aún no tienes selector
			}
			else // EXIT
			{
				exitRequested = true;
			}
		}

		// Marcamos que debemos bloquear gameplay este frame
		introWantsBlock = true;
	}

	// --- Estado GAMEOVER: menú con selectores ---
	if (gameState == GameState::GAMEOVER)
	{
		// Navegación teclado
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_DOWN ||
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_DOWN)
		{
			gameOverOption = (gameOverOption == GameOverOption::RETRY)
				? GameOverOption::BACK_TO_TITLE 
				: GameOverOption::RETRY;
		}

		// Mouse hover/click
		int mx, my;
		Engine::GetInstance().input->GetMousePosition(mx, my);

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		const int btnW = 340;
		const int btnH = 60;
		const int gap = 16;

		int startX = (winW - btnW) / 2;
		int startY = (winH / 2) + 120;

		SDL_Rect retryRect = { startX, startY, btnW, btnH };
		SDL_Rect introRect = { startX, startY + (btnH + gap), btnW, btnH };

		auto inside = [&](const SDL_Rect& r) {
			return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
			};

		if (inside(retryRect)) gameOverOption = GameOverOption::RETRY;
		if (inside(introRect)) gameOverOption = GameOverOption::BACK_TO_TITLE;

		bool activate =
			(Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN) ||
			(Engine::GetInstance().input->GetMouseButtonDown(1) == KEY_DOWN);

		if (activate)
		{
			if (gameOverOption == GameOverOption::RETRY)
			{
				gameOverActive = false;
				RestartLevel();
			}
			else // BACK_TO_INTRO
			{
				gameOverActive = false;

				// Dejamos el salto listo aunque INTRO aún no esté implementado
				gameState = GameState::TITLE;

				// Cuando implementemos Intro, aquí pondremos su música
				// Engine::GetInstance().audio->PlayMusic(MUSIC_INTRO);
			}
		}

		return true; // bloquear gameplay en GAMEOVER
	}


	if (gameState == GameState::PAUSED) {

		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_DOWN)
		{
			int idx = (int)pauseOption;
			idx = (idx - 1 + 4) % 4;
			pauseOption = (PauseOption)idx;
		}
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_DOWN)
		{
			int idx = (int)pauseOption;
			idx = (idx + 1) % 4;
			pauseOption = (PauseOption)idx;
		}

		// Mouse: hover/click en botones (rectángulos)
		int mx, my;
		Engine::GetInstance().input->GetMousePosition(mx, my);

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		// Layout centrado
		const int btnW = 320;
		const int btnH = 55;
		const int gap = 14;

		int startX = (winW - btnW) / 2;
		int startY = (winH / 2) - (btnH * 2) - (gap * 2);

		SDL_Rect buttons[4];
		for (int i = 0; i < 4; ++i)
		{
			buttons[i] = { startX, startY + i * (btnH + gap), btnW, btnH };

			// Hover -> selecciona
			if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
				my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
			{
				pauseOption = (PauseOption)i;
			}
		}

		bool activate =
			(Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN) ||
			(Engine::GetInstance().input->GetMouseButtonDown(1) == KEY_DOWN);

		if (activate)
		{
			switch (pauseOption)
			{
			case PauseOption::RESUME:
				gameState = GameState::PLAYING;
				break;

			case PauseOption::SETTINGS:
				showSettingsFromPause = !showSettingsFromPause; // placeholder visual
				LOG("SETTINGS (placeholder): aqui luego abriremos el menu real con sliders/checkbox.");
				break;

			case PauseOption::BACK_TO_TITLE:
				gameState = GameState::TITLE;
				showSettingsFromPause = false;
				pauseOption = PauseOption::RESUME;
				Engine::GetInstance().audio->PlayMusic(MUSIC_LEVEL1);
				break;

			case PauseOption::EXIT:
				exitRequested = true;
				break;
			}
		}

		return true;
	}

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y -= (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y += (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x -= (int)ceil(camSpeed * dt);
	
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x += (int)ceil(camSpeed * dt);

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

void Scene::RestartLevel()
{
	LOG("RestartLevel(): reiniciando nivel sin usar saves...");

	// 1) Borrar entidades excepto PLAYER (y hoguera si la quieres conservar como objeto del mapa)
	auto& list = Engine::GetInstance().entityManager->entities;
	for (auto it = list.begin(); it != list.end(); )
	{
		if (*it && (*it)->type != EntityType::PLAYER)
		{
			(*it)->CleanUp();
			it = list.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 2) Recargar mapa (esto recreará colliders/objetos del mapa)
	Engine::GetInstance().map->Load("Assets/Maps/", "Level01.tmx");

	// 3) Reset del jugador a spawn inicial
	//    Si tienes spawnPosition ya seteada al inicio del juego, úsala.
	//    Si no, usa la posición actual como fallback.
	Vector2D spawn = player->spawnPosition;
	if (spawn.getX() == 0 && spawn.getY() == 0)
		spawn = player->position;

	player->position = spawn;
	if (player->pbody)
		player->pbody->SetPosition((int)spawn.getX(), (int)spawn.getY());

	// Reset de HP (ajusta si quieres otra lógica)
	player->hp = player->maxHp;

	// 4) Reset cámara (opcional: centrado básico)
	Engine::GetInstance().render->camera.x = 0;
	Engine::GetInstance().render->camera.y = 0;

	// 5) Volver a jugar
	gameState = GameState::PLAYING;

	// Música del nivel
	Engine::GetInstance().audio->PlayMusic(MUSIC_LEVEL1);
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
	gameState = GameState::GAMEOVER;
	gameOverActive = true;
	gameOverOption = GameOverOption::RETRY;

	// Música de pausa o una específica de gameover (si tienes)
	Engine::GetInstance().audio->PlayMusic(MUSIC_GAMEOVER);
}

void Scene::DrawGameOver()
{
	if (!gameOverActive || gameOverTexture == nullptr)
		return;

	// Guardar cámara
	int oldCamX = Engine::GetInstance().render->camera.x;
	int oldCamY = Engine::GetInstance().render->camera.y;

	Engine::GetInstance().render->camera.x = 0;
	Engine::GetInstance().render->camera.y = 0;

	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Dibujar imagen GAMEOVER en modo "cover"
	float texW = 0.0f, texH = 0.0f;
	if (SDL_GetTextureSize(gameOverTexture, &texW, &texH))
	{
		float scaleX = (texW > 0.0f) ? ((float)winW / texW) : 1.0f;
		float scaleY = (texH > 0.0f) ? ((float)winH / texH) : 1.0f;
		float coverScale = (scaleX > scaleY) ? scaleX : scaleY;

		float dstW = texW * coverScale;
		float dstH = texH * coverScale;
		float dstX = ((float)winW - dstW) * 0.5f;
		float dstY = ((float)winH - dstH) * 0.5f;

		SDL_FRect dstRect{ dstX, dstY, dstW, dstH };
		SDL_RenderTexture(Engine::GetInstance().render->renderer, gameOverTexture, nullptr, &dstRect);
	}

	// Overlay oscuro
	SDL_Rect full = { 0, 0, winW, winH };
	Engine::GetInstance().render->DrawRectangle(full, 0, 0, 0, 120, true, false);

	// Botones (sin texto aún)
	const int btnW = 340;
	const int btnH = 60;
	const int gap = 16;

	int startX = (winW - btnW) / 2;
	int startY = (winH / 2) + 120;

	SDL_Rect retryRect = { startX, startY, btnW, btnH };
	SDL_Rect introRect = { startX, startY + (btnH + gap), btnW, btnH };

	auto drawBtn = [&](const SDL_Rect& r, bool selected)
		{
			if (selected)
				Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 220, true, false);
			else
				Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 140, true, false);

			Engine::GetInstance().render->DrawRectangle(r, 0, 0, 0, 255, false, false);
		};

	drawBtn(retryRect, gameOverOption == GameOverOption::RETRY);
	drawBtn(introRect, gameOverOption == GameOverOption::BACK_TO_TITLE);

	// Restaurar cámara
	Engine::GetInstance().render->camera.x = oldCamX;
	Engine::GetInstance().render->camera.y = oldCamY;
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

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F12) == KEY_DOWN)
		ret = false;

	if (gameState == GameState::PAUSED)
	{
		if (pauseTexture != nullptr)
		{
			// Guardar cámara actual
			int oldCamX = Engine::GetInstance().render->camera.x;
			int oldCamY = Engine::GetInstance().render->camera.y;

			// Forzar cámara a 0 para dibujar UI
			Engine::GetInstance().render->camera.x = 0;
			Engine::GetInstance().render->camera.y = 0;

			// Tamaño de la ventana (en píxeles)
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);

			// Tamaño real de la textura
			float texW = 0.0f, texH = 0.0f;
			if (!SDL_GetTextureSize(pauseTexture, &texW, &texH))
			{
				LOG("SDL_GetTextureSize failed for pauseTexture: %s", SDL_GetError());
			}
			else
			{
				// Escala uniforme para que "quepa" dentro de la ventana, manteniendo proporción
				float scaleX = (texW > 0.0f) ? ((float)winW / texW) : 1.0f;
				float scaleY = (texH > 0.0f) ? ((float)winH / texH) : 1.0f;
				float fitScale = (scaleX > scaleY) ? scaleX : scaleY;
				fitScale *= 1.0f; // dejar un margen del 20%

				float dstW = texW * fitScale;
				float dstH = texH * fitScale;

				float dstX = ((float)winW - dstW) * 0.5f;
				float dstY = ((float)winH - dstH) * 0.5f;

				SDL_FRect dstRect{ dstX, dstY, dstW, dstH };

				// Dibujar directamente con SDL para poder controlar destino (tamaño/posición)
				SDL_RenderTexture(Engine::GetInstance().render->renderer, pauseTexture, nullptr, &dstRect);
			}

			// Restaurar cámara
			Engine::GetInstance().render->camera.x = oldCamX;
			Engine::GetInstance().render->camera.y = oldCamY;
		}
		else
		{
			LOG("pauseTexture is nullptr (no se puede dibujar la pausa).");
		}
		// Overlay oscuro para separar el fondo del UI
		int w, h;
		Engine::GetInstance().window->GetWindowSize(w, h);
		SDL_Rect full = { 0, 0, w, h };
		Engine::GetInstance().render->DrawRectangle(full, 0, 0, 0, 120, true, false);

		// Botones
		const int btnW = 320;
		const int btnH = 55;
		const int gap = 14;

		int startX = (w - btnW) / 2;
		int startY = (h / 2) - (btnH * 2) - (gap * 2);

		for (int i = 0; i < 4; ++i)
		{
			SDL_Rect r = { startX, startY + i * (btnH + gap), btnW, btnH };

			bool selected = ((int)pauseOption == i);

			// fondo botón
			if (selected)
				Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 220, true, false);
			else
				Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 140, true, false);

			// borde
			Engine::GetInstance().render->DrawRectangle(r, 0, 0, 0, 255, false, false);
		}

		// Mini panel de "settings" placeholder
		if (showSettingsFromPause)
		{
			SDL_Rect panel = { (w - 520) / 2, startY + 4 * (btnH + gap) + 10, 520, 120 };
			Engine::GetInstance().render->DrawRectangle(panel, 0, 0, 0, 200, true, false);
			Engine::GetInstance().render->DrawRectangle(panel, 255, 255, 255, 255, false, false);
		}
	}

	if (gameState == GameState::INTRO && introTexture != nullptr)
	{
		int oldCamX = Engine::GetInstance().render->camera.x;
		int oldCamY = Engine::GetInstance().render->camera.y;
		Engine::GetInstance().render->camera.x = 0;
		Engine::GetInstance().render->camera.y = 0;

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		float texW = 0.0f, texH = 0.0f;
		if (SDL_GetTextureSize(introTexture, &texW, &texH))
		{
			float scaleX = (texW > 0.0f) ? ((float)winW / texW) : 1.0f;
			float scaleY = (texH > 0.0f) ? ((float)winH / texH) : 1.0f;
			float coverScale = (scaleX > scaleY) ? scaleX : scaleY;

			float dstW = texW * coverScale;
			float dstH = texH * coverScale;
			float dstX = ((float)winW - dstW) * 0.5f;
			float dstY = ((float)winH - dstH) * 0.5f;

			SDL_FRect dstRect{ dstX, dstY, dstW, dstH };
			SDL_RenderTexture(Engine::GetInstance().render->renderer, introTexture, nullptr, &dstRect);
		}

		// Highlight botones START / EXIT
			const int btnW = 360;
		const int btnH = 75;
		const int gap = 18;

		int startX = (winW - btnW) / 2;
		int startY_Start = (winH / 2) + 140; // START más arriba
		int startY_Exit = (winH / 2) + 160 + btnH + gap;

		SDL_Rect startRect = { startX, startY_Start, btnW, btnH };
		SDL_Rect exitRect = { startX, startY_Exit,  btnW, btnH };

		auto drawBtn = [&](const SDL_Rect& r, bool selected)
			{
				if (selected)
				{
					// Borde blanco brillante
					Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 255, false, false);

					// Glow suave (opcional pero queda muy bien)
					SDL_Rect glow = { r.x - 2, r.y - 2, r.w + 4, r.h + 4 };
					Engine::GetInstance().render->DrawRectangle(glow, 255, 255, 255, 120, false, false);
				}
				else
				{
					// Borde muy sutil cuando no está seleccionado
					Engine::GetInstance().render->DrawRectangle(r, 255, 255, 255, 80, false, false);
				}
			};

		drawBtn(startRect, introOption == IntroOption::START);
		drawBtn(exitRect, introOption == IntroOption::EXIT);

		Engine::GetInstance().render->camera.x = oldCamX;
		Engine::GetInstance().render->camera.y = oldCamY;
	}

	if (exitRequested)
		ret = false;

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

	if (gameOverTexture)
	{
		Engine::GetInstance().textures->UnLoad(gameOverTexture);
		gameOverTexture = nullptr;
	}

	if (introTexture)
	{
		Engine::GetInstance().textures->UnLoad(introTexture);
		introTexture = nullptr;
	}

	LOG("Freeing scene");

	return true;
}



