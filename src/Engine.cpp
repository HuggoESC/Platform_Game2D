#include <iostream>
#include <sstream>
#include <iomanip>

#include "Engine.h"
#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "Textures.h"
#include "Audio.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Map.h"
#include "Physics.h"
#include "Log.h"

Engine::Engine() {

	LOG("Constructor Engine::Engine");

	Timer timer = Timer();
	startupTime = Timer();
	frameTime = PerfTimer();
	lastSecFrameTime = PerfTimer();
	frames = 0;
	
	window = std::make_shared<Window>();
	input = std::make_shared<Input>();
	render = std::make_shared<Render>();
	textures = std::make_shared<Textures>();
	audio = std::make_shared<Audio>();
	physics = std::make_shared<Physics>();
	scene = std::make_shared<Scene>();
	map = std::make_shared<Map>();
	entityManager = std::make_shared<EntityManager>();

	AddModule(std::static_pointer_cast<Module>(window));
	AddModule(std::static_pointer_cast<Module>(input));
	AddModule(std::static_pointer_cast<Module>(textures));
	AddModule(std::static_pointer_cast<Module>(audio));
	AddModule(std::static_pointer_cast<Module>(physics));
	AddModule(std::static_pointer_cast<Module>(map));
	AddModule(std::static_pointer_cast<Module>(scene));
	AddModule(std::static_pointer_cast<Module>(entityManager));
	AddModule(std::static_pointer_cast<Module>(render));

	LOG("Timer App Constructor: %f", timer.ReadMSec());
}

Engine& Engine::GetInstance() {
	static Engine instance; 
	return instance;
}

void Engine::AddModule(std::shared_ptr<Module> module){
	module->Init();
	moduleList.push_back(module);
}

bool Engine::Awake() {

	Timer timer = Timer();

	LOG("Engine::Awake");

	LoadConfig();
	gameTitle = configFile.child("config").child("engine").child("title").child_value();
	targetFrameRate = configFile.child("config").child("engine").child("targetFrameRate").attribute("value").as_int();

	bool result = true;
	for (const auto& module : moduleList) {
		module->LoadParameters(configFile.child("config").child(module.get()->name.c_str()));
		result =  module->Awake();

		if (!result) {
			break;
		}
	}

	LOG("Timer App Awake(): %f", timer.ReadMSec());

	return result;
}

bool Engine::Start() {

	Timer timer = Timer();

	LOG("Engine::Start");

	bool result = true;
	for (const auto& module : moduleList) {
		result = module->Start();
		if (!result) {
			break;
		}
	}

	LOG("Timer App CleanUp(): %f", timer.ReadMSec());
	
	return result;
}

bool Engine::Update() {

	bool ret = true;
	PrepareUpdate();

	if (input->GetWindowEvent(WE_QUIT) == true)
		ret = false;

	if (ret == true)
		ret = PreUpdate();

	if (ret == true)
		ret = DoUpdate();

	if (ret == true)
		ret = PostUpdate();

	FinishUpdate();
	return ret;
}

bool Engine::CleanUp() {

	Timer timer = Timer();

	LOG("Engine::CleanUp");

	bool result = true;
	for (const auto& module : moduleList) {
		result = module->CleanUp();
		if (!result) {
			break;
		}
	}

	LOG("Timer App CleanUp(): %f", timer.ReadMSec());

	return result;
}

void Engine::PrepareUpdate()
{
	frameTime.Start();
}

void Engine::FinishUpdate()
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F11) == KEY_DOWN) {
		if (targetFrameRate == 60) {
			targetFrameRate = 30;
		} 
		else{
			targetFrameRate = 60;
		}
	}
	double currentDt = frameTime.ReadMs();
	float maxFrameDuration = 1000.0f / targetFrameRate;
	if (targetFrameRate > 0 && currentDt < maxFrameDuration) {

		Uint32 delay = (Uint32)(maxFrameDuration - currentDt);
		PerfTimer delayTimer = PerfTimer();
		SDL_Delay(delay);
	   
	}

	frameCount++;

	secondsSinceStartup = startupTime.ReadSec();

	dt = (float)frameTime.ReadMs();

	lastSecFrameCount++;

	if (lastSecFrameTime.ReadMs() > 1000) {
		lastSecFrameTime.Start();
		averageFps = (averageFps + lastSecFrameCount) / 2;
		framesPerSecond = lastSecFrameCount;
		lastSecFrameCount = 0;
	}

	std::stringstream ss;
	ss << gameTitle << ": Av.FPS: " << std::fixed << std::setprecision(2) << averageFps
		<< " Last sec frames: " << framesPerSecond
		<< " Last dt: " << std::fixed << std::setprecision(3) << dt
		<< " Time since startup: " << secondsSinceStartup
		<< " Frame Count: " << frameCount;

	std::string titleStr = ss.str();

	window.get()->SetTitle(titleStr.c_str());
}


bool Engine::PreUpdate()
{
	bool result = true;
	for (const auto& module : moduleList) {
		result = module->PreUpdate();
		if (!result) {
			break;
		}
	}

	return result;
}

bool Engine::DoUpdate()
{
	bool result = true;
	for (const auto& module : moduleList) {
		result = module->Update(dt);
		if (!result) {
			break;
		}
	}

	return result;
}

bool Engine::PostUpdate()
{
	bool result = true;
	for (const auto& module : moduleList) {
		result = module->PostUpdate();
		if (!result) {
			break;
		}
	}

	return result;
}

bool Engine::LoadConfig()
{
	bool ret = true;
	pugi::xml_parse_result result = configFile.load_file("config.xml");
	if (result)
	{
		LOG("config.xml parsed without errors");
	}
	else
	{
		LOG("Error loading config.xml: %s", result.description());
	}

	return ret;
}


