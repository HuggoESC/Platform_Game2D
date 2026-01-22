#pragma once

#include "Module.h"
#include <SDL3/SDL.h>

class Window : public Module
{
public:

	Window();

	virtual ~Window();

	bool Awake();

	bool CleanUp();

	void SetTitle(const char* title);

	void GetWindowSize(int& width, int& height) const;

	int GetScale() const;

	void ToggleFullscreen();

	bool IsFullscreen() const;

	float GetRenderScale() const;

public:
	SDL_Window* window;

	std::string title;
	int width = 1280;
	int height = 720;
	int scale = 1;
	bool fullscreen = false;
	float renderScale = 1.0f;
};
