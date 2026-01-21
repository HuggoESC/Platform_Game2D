#include "Window.h"
#include "Log.h"
#include "Engine.h"
#include "Render.h"

Window::Window() : Module()
{
	window = NULL;
	name = "window";
}

// Destructor
Window::~Window()
{
}

// Called before render is available
bool Window::Awake()
{
	LOG("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		Uint32 flags = 0;
		bool fullscreen = configParameters.child("fullscreen").attribute("value").as_bool();
		bool borderless = configParameters.child("borderless").attribute("value").as_bool();
		bool resizable = configParameters.child("resizable").attribute("value").as_bool();
		bool fullscreen_window = configParameters.child("fullscreen_window").attribute("value").as_bool();

		//TODO Get the values from the config file
		width = configParameters.child("resolution").attribute("width").as_int();
		height = configParameters.child("resolution").attribute("height").as_int();
		scale = configParameters.child("resolution").attribute("scale").as_int();

		if (fullscreen == true)        flags |= SDL_WINDOW_FULLSCREEN;
		if (borderless == true)        flags |= SDL_WINDOW_BORDERLESS;
		if (resizable == true)         flags |= SDL_WINDOW_RESIZABLE;

		window = SDL_CreateWindow("Platform Game", width, height, flags);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (fullscreen_window == true)
			{
				SDL_SetWindowFullscreenMode(window, nullptr); 
				SDL_SetWindowFullscreen(window, true);
			}
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_ShowWindow(window);
		}
	}

	return ret;
}

// Called before quitting
bool Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	// Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void Window::SetTitle(const char* new_title)
{
	SDL_SetWindowTitle(window, new_title);
}

void Window::GetWindowSize(int& width, int& height) const
{
	width = this->width;
	height = this->height;
}

int Window::GetScale() const
{
	return scale;
}

void Window::ToggleFullscreen()
{
	if (window == NULL) return;

	fullscreen = !fullscreen;

	if (fullscreen)
	{
		// Set to fullscreen
		SDL_SetWindowFullscreenMode(window, nullptr);
		SDL_SetWindowFullscreen(window, true);
		
		// Get the fullscreen display size
		const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
		if (mode)
		{
			LOG("Fullscreen mode: %dx%d", mode->w, mode->h);
			// Calculate scale based on display and game resolution
			float scaleX = (float)mode->w / width;
			float scaleY = (float)mode->h / height;
			renderScale = (scaleX < scaleY) ? scaleX : scaleY;
			
			// Set renderer scale to upscale the game
			SDL_SetRenderScale(Engine::GetInstance().render->renderer, renderScale, renderScale);
			LOG("Render scale set to: %.2f", renderScale);
		}
		
		LOG("Fullscreen enabled");
	}
	else
	{
		// Exit fullscreen
		SDL_SetWindowFullscreen(window, false);
		
		// Restore window size
		SDL_SetWindowSize(window, width, height);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		
		// Reset renderer scale and track it
		renderScale = 1.0f;
		SDL_SetRenderScale(Engine::GetInstance().render->renderer, 1.0f, 1.0f);
		LOG("Render scale reset to: 1.0");
		
		LOG("Fullscreen disabled");
	}
}

bool Window::IsFullscreen() const
{
	return fullscreen;
}

float Window::GetRenderScale() const
{
	return renderScale;
}
