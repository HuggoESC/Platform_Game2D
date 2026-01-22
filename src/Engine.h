#pragma once

#include <memory>
#include <list>
#include "Module.h"
#include "Timer.h"
#include "PerfTimer.h"
#include "pugixml.hpp"

class Window;
class Input;
class Render;
class Textures;
class Audio;
class Scene;
class EntityManager;
class Map;
class Physics;
class Engine
{
public:

	static Engine& GetInstance();

	void AddModule(std::shared_ptr<Module> module);

	bool Awake();

	bool Start();

	bool Update();

	bool CleanUp();

	float GetDt() const {
		return dt;
	}

private:

	Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void PrepareUpdate();

	void FinishUpdate();

	bool PreUpdate();

	bool DoUpdate();

	bool PostUpdate();

	bool LoadConfig();

	std::list<std::shared_ptr<Module>> moduleList;

public:

	enum EngineState
	{
		CREATE = 1,
		AWAKE,
		START,
		LOOP,
		CLEAN,
		FAIL,
		EXIT
	};

	// Modules
	std::shared_ptr<Window> window;
	std::shared_ptr<Input> input;
	std::shared_ptr<Render> render;
	std::shared_ptr<Textures> textures;
	std::shared_ptr<Audio> audio;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<EntityManager> entityManager;
	std::shared_ptr<Map> map;
	std::shared_ptr<Physics> physics;


private: 

	float dt; 
	int frames;

	Timer startupTime;
	PerfTimer frameTime;
	PerfTimer lastSecFrameTime;

	int frameCount = 0;
	int framesPerSecond = 0;
	int lastSecFrameCount = 0;

	float averageFps = 0.0f;
	int secondsSinceStartup = 0;

	int targetFrameRate = 60;

	std::string gameTitle = "Platformer Game";

	pugi::xml_document configFile;

};