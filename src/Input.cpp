#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "Log.h"

#define MAX_KEYS 300

Input::Input() : Module()
{
	name = "input";

	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
	memset(windowEvents, 0, sizeof(windowEvents));
	mouseMotionX = mouseMotionY = mouseX = mouseY = 0;
}

Input::~Input()
{
	delete[] keyboard;
}

bool Input::Awake()
{
	LOG("Init SDL input event system");
	bool ret = true;

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) != true)
	{
		LOG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Input::Start()
{
	SDL_StopTextInput(Engine::GetInstance().window->window);
	return true;
}

bool Input::PreUpdate()
{
	static SDL_Event event;

	int numKeys = 0;
	const bool* keys = SDL_GetKeyboardState(&numKeys);

	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i] == 1)
		{
			if (keyboard[i] == KEY_IDLE)
				keyboard[i] = KEY_DOWN;
			else
				keyboard[i] = KEY_REPEAT;
		}
		else
		{
			if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
				keyboard[i] = KEY_UP;
			else
				keyboard[i] = KEY_IDLE;
		}
	}

	for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
	{
		if (mouseButtons[i] == KEY_DOWN)
			mouseButtons[i] = KEY_REPEAT;

		if (mouseButtons[i] == KEY_UP)
			mouseButtons[i] = KEY_IDLE;
	}

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			windowEvents[WE_QUIT] = true;
			break;

		case SDL_EVENT_WINDOW_HIDDEN:
		case SDL_EVENT_WINDOW_MINIMIZED:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			windowEvents[WE_HIDE] = true;
			break;

		case SDL_EVENT_WINDOW_SHOWN:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_RESTORED:
			windowEvents[WE_SHOW] = true;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_DOWN;
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_UP;
			break;

		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Engine::GetInstance().window->GetScale();
			float renderScale = Engine::GetInstance().window->GetRenderScale();

			float totalScale = scale * renderScale;

			mouseMotionX = (int)(event.motion.xrel / totalScale);
			mouseMotionY = (int)(event.motion.yrel / totalScale);
			mouseX = (int)(event.motion.x / totalScale);
			mouseY = (int)(event.motion.y / totalScale);
		}
		break;
		}
	}

	return true;
}

bool Input::CleanUp()
{
	LOG("Quitting SDL event subsystem");
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}

void Input::GetMousePosition(int& x, int& y)
{
	x = mouseX;
	y = mouseY;
}

void Input::GetMouseMotion(int& x, int& y)
{
	x = mouseMotionX;
	y = mouseMotionY;
}
