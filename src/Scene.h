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
	INTRO,
	LEVELSELECTOR,
	PLAYING,
	GAMEOVER,
	PAUSED,
	TITLE
};

// --- GameOver menu ---
enum class GameOverOption { RETRY, BACK_TO_TITLE };

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
	bool IsPaused() const { return gameState == GameState::PAUSED || gameState == GameState::TITLE; }

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

	void RestartLevel();

	SaveMode saveMode = SaveMode::NONE;

private:

	// Declare a Player attribute
	std::shared_ptr<Player> player;

	GameState gameState = GameState::PLAYING;

	// --- GameOver menu ---
	GameOverOption gameOverOption = GameOverOption::RETRY;
	SDL_Texture* gameOverTexture = nullptr;

	SDL_Texture* pauseTexture = nullptr;

	// --- Level selector ---
	enum class LevelSelOption { LEVEL1, LEVEL2, BACK };
	LevelSelOption levelSelOption = LevelSelOption::LEVEL1;

	SDL_Texture* levelSelectorTexture = nullptr;

	bool levelSelectorInputLock = false;

	// Nivel actual
	int currentLevel = 1;

	// Helpers
	void LoadLevel(int level);

	// --- Pause menu ---
	enum class PauseOption { RESUME, SETTINGS, BACK_TO_TITLE, EXIT };
	PauseOption pauseOption = PauseOption::RESUME;

	// --- Intro menu ---
	enum class IntroOption { START, EXIT };
	IntroOption introOption = IntroOption::START;

	SDL_Texture* introTexture = nullptr;
	bool exitRequested = false;
	bool showSettingsFromPause = false; // placeholder de momento

	bool pauseMusicPlaying = false;	// para pausar/reanudar música al pausar

	bool pendingSave = false;
	bool pendingLoad = false;
	int pendingSlot = 1;

	GameState lastState = GameState::PLAYING;

	int loadNotificationSlot = 0;
	float loadNotificationTimer = 0.0f;

	bool gameOverActive = false;
	float gameOverTimer = 0.0f;

};