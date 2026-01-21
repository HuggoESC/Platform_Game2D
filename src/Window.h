#pragma once

#include "Module.h"
#include <SDL3/SDL.h>

class Window : public Module
{
public:

	Window();

	// Destructor
	virtual ~Window();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Changae title
	void SetTitle(const char* title);

	// Retrive window size
	void GetWindowSize(int& width, int& height) const;

	// Retrieve window scale
	int GetScale() const;

	// Toggle fullscreen mode
	void ToggleFullscreen();

	// Check if currently in fullscreen
	bool IsFullscreen() const;

	// Get the current render scale (different from window scale when fullscreen)
	float GetRenderScale() const;

public:
	// The window we'll be rendering to
	SDL_Window* window;

	std::string title;
	int width = 1280;
	int height = 720;
	int scale = 1;
	bool fullscreen = false;
	float renderScale = 1.0f;
};
