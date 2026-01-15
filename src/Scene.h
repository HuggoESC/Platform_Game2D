#pragma once

#include "Module.h"
#include "Player.h"
#include "LifeUP.h"

struct SDL_Texture;

enum class SaveMode
{
	NONE,
	SAVE_MENU,
	LOAD_MENU
};

enum class GameState
{
	PLAYING,
	GAMEOVER,
	PAUSED,
	TITLE
};

class Scene : public Module
{
public:

	Scene();

	// Destructor
	virtual ~Scene();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before all Updates
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called before all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Save and Load
	bool SaveGame();
	bool LoadGame();

	// Pausar el juego
	bool IsPaused() const { return gameState == GameState::PAUSED; }

	// Save and Load from a specific slot
	bool SaveGameToSlot(int slot);
	bool LoadGameFromSlot(int slot);
	
	void ShowLoadNotification(int slot);
	void DrawLoadNotification();

	void TriggerGameOver();
	void DrawGameOver();

	void SetCheckpoint(const Vector2D& pos);
	void RequestSave(int slot);
	void RequestLoad(int slot);

	SaveMode saveMode = SaveMode::NONE;

private:

	// Declare a Player attribute
	std::shared_ptr<Player> player;

	GameState gameState = GameState::PLAYING;
	
	SDL_Texture* pauseTexture = nullptr;

	bool pendingSave = false;
	bool pendingLoad = false;
	int pendingSlot = 1;

	int loadNotificationSlot = 0;
	float loadNotificationTimer = 0.0f;

	bool gameOverActive = false;
	float gameOverTimer = 0.0f;

};