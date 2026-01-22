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

enum class GameOverOption { RETRY, BACK_TO_TITLE };

class Scene : public Module
{
public:

	Scene();

	virtual ~Scene();

	bool Awake();

	bool Start();

	bool PreUpdate();

	bool Update(float dt);

	bool PostUpdate();

	bool CleanUp();

	bool SaveGame();
	bool LoadGame();

	bool IsPaused() const { return gameState == GameState::PAUSED || gameState == GameState::TITLE; }

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
	void RequestLevelChange(int level, const Vector2D& spawnPos);

	SaveMode saveMode = SaveMode::NONE;

private:

	std::shared_ptr<Player> player;

	GameState gameState = GameState::PLAYING;

	GameOverOption gameOverOption = GameOverOption::RETRY;
	SDL_Texture* gameOverTexture = nullptr;

	SDL_Texture* pauseTexture = nullptr;

	enum class LevelSelOption { LEVEL1, LEVEL2, BACK };
	LevelSelOption levelSelOption = LevelSelOption::LEVEL1;

	SDL_Texture* levelSelectorTexture = nullptr;

	bool levelSelectorInputLock = false;

	int currentLevel = 1;

	void LoadLevel(int level);

	enum class PauseOption { RESUME, SETTINGS, BACK_TO_TITLE, EXIT };
	PauseOption pauseOption = PauseOption::RESUME;

	enum class IntroOption { START, EXIT };
	IntroOption introOption = IntroOption::START;

	SDL_Texture* introTexture = nullptr;
	bool exitRequested = false;
	bool showSettingsFromPause = false; 

	bool pauseMusicPlaying = false;	

	bool pendingSave = false;
	bool pendingLoad = false;
	int pendingSlot = 1;

	GameState lastState = GameState::PLAYING;

	int loadNotificationSlot = 0;
	float loadNotificationTimer = 0.0f;

	bool gameOverActive = false;
	float gameOverTimer = 0.0f;

	bool comingFromLevelTransition = false;	
	bool pendingLevelChange = false;
	int pendingNextLevel = 1;
	Vector2D pendingSpawn = Vector2D(96, 650);

};