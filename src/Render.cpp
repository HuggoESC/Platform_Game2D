#include "Engine.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "Input.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 0;
}

// Destructor
Render::~Render()
{
}

// Called before render is available
bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	int scale = Engine::GetInstance().window->GetScale();
	SDL_Window* window = Engine::GetInstance().window->window;

	renderer = SDL_CreateRenderer(window, nullptr);

	if (renderer == NULL)
	{
		LOG("Could not create the renderer! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		if (configParameters.child("vsync").attribute("value").as_bool())
		{
			if (!SDL_SetRenderVSync(renderer, 1))
			{
				LOG("Warning: could not enable vsync: %s", SDL_GetError());
			}
			else
			{
				LOG("Using vsync");
			}
		}

		camera.w = Engine::GetInstance().window->width * scale;
		camera.h = Engine::GetInstance().window->height * scale;
		camera.x = 0;
		camera.y = 0;
	}

	return ret;
}

// Called before the first frame
bool Render::Start()
{
	LOG("render start");
	// back background
	if (!SDL_GetRenderViewport(renderer, &viewport))
	{
		LOG("SDL_GetRenderViewport failed: %s", SDL_GetError());
	}
	return true;
}

// Called each loop iteration
bool Render::PreUpdate()
{
	SDL_RenderClear(renderer);
	return true;
}

bool Render::Update(float dt)
{
	HandleInput();
	return true;
}

bool Render::PostUpdate()
{
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.g, background.a);
	SDL_RenderPresent(renderer);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	SDL_DestroyRenderer(renderer);
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
}

void Render::SetViewPort(const SDL_Rect& rect)
{
	SDL_SetRenderViewport(renderer, &rect);
}

void Render::ResetViewPort()
{
	SDL_SetRenderViewport(renderer, &viewport);
}

void Render::HandleInput()
{
	// Toggle fullscreen with O key
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_O) == KEY_DOWN)
	{
		Engine::GetInstance().window->ToggleFullscreen();
	}
}

// Blit to screen
bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float speed, double angle, int pivotX, int pivotY) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_FRect rect;
	rect.x = (float)((int)(camera.x * speed) + x * scale);
	rect.y = (float)((int)(camera.y * speed) + y * scale);

	if (section != nullptr)
	{
		rect.w = (float)(section->w * scale);
		rect.h = (float)(section->h * scale);
	}
	else
	{
		float tw, th;
		if (!SDL_GetTextureSize(texture, &tw, &th))
		{
			LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
			return false;
		}
		rect.w = tw * scale;
		rect.h = th * scale;
	}

	const SDL_FRect* src = nullptr;
	SDL_FRect srcRect;
	if (section != nullptr)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = nullptr;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX;
		pivot.y = (float)pivotY;
		p = &pivot;
	}

	// SDL3: sin flip
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, SDL_FLIP_NONE) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section,
	float speed, double angle, int pivotX, int pivotY, bool flipX) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_FRect rect;
	rect.x = (float)((int)(camera.x * speed) + x * scale);
	rect.y = (float)((int)(camera.y * speed) + y * scale);

	if (section != nullptr)
	{
		rect.w = (float)(section->w * scale);
		rect.h = (float)(section->h * scale);
	}
	else
	{
		float tw, th;
		if (!SDL_GetTextureSize(texture, &tw, &th))
		{
			LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
			return false;
		}
		rect.w = tw * scale;
		rect.h = th * scale;
	}

	const SDL_FRect* src = nullptr;
	SDL_FRect srcRect;
	if (section != nullptr)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = nullptr;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX;
		pivot.y = (float)pivotY;
		p = &pivot;
	}

	SDL_FlipMode flip = flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, flip) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawTextureScaled(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float scale)
{
	if (texture == nullptr) return false;

	SDL_FRect src;
	SDL_FRect dst;

	// SRC (SDL3 usa FRect). Convertimos desde SDL_Rect si hay section.
	if (section != nullptr)
	{
		src.x = (float)section->x;
		src.y = (float)section->y;
		src.w = (float)section->w;
		src.h = (float)section->h;

		dst.w = (float)section->w * scale;
		dst.h = (float)section->h * scale;
	}
	else
	{
		// Si no hay section, usamos tamaño completo de textura
		float w = 0, h = 0;
		SDL_GetTextureSize(texture, &w, &h);

		src.x = 0;
		src.y = 0;
		src.w = w;
		src.h = h;

		dst.w = w * scale;
		dst.h = h * scale;
	}

	// DST (posición + cámara)
	dst.x = (float)(x + camera.x);
	dst.y = (float)(y + camera.y);

	if (SDL_RenderTexture(renderer, texture, &src, &dst) < 0)
	{
		LOG("SDL_RenderTexture error: %s", SDL_GetError());
		return false;
	}

	return true;
}


bool Render::DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool filled, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_FRect rec;
	if (use_camera)
	{
		rec.x = (float)((int)(camera.x + rect.x * scale));
		rec.y = (float)((int)(camera.y + rect.y * scale));
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}
	else
	{
		rec.x = (float)(rect.x * scale);
		rec.y = (float)(rect.y * scale);
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}

	int result = (filled ? SDL_RenderFillRect(renderer, &rec) : SDL_RenderRect(renderer, &rec)) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderFillRect/SDL_RenderRect error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	float X1, Y1, X2, Y2;

	if (use_camera)
	{
		X1 = (float)(camera.x + x1 * scale);
		Y1 = (float)(camera.y + y1 * scale);
		X2 = (float)(camera.x + x2 * scale);
		Y2 = (float)(camera.y + y2 * scale);
	}
	else
	{
		X1 = (float)(x1 * scale);
		Y1 = (float)(y1 * scale);
		X2 = (float)(x2 * scale);
		Y2 = (float)(y2 * scale);
	}

	int result = SDL_RenderLine(renderer, X1, Y1, X2, Y2) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderLine error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawCircle(int x, int y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	int result = -1;
	SDL_FPoint points[360];

	float factor = (float)M_PI / 180.0f;

	float cx = (float)((use_camera ? camera.x : 0) + x * scale);
	float cy = (float)((use_camera ? camera.y : 0) + y * scale);

	for (int i = 0; i < 360; ++i)
	{
		points[i].x = cx + (float)(radius * cos(i * factor));
		points[i].y = cy + (float)(radius * sin(i * factor));
	}

	result = SDL_RenderPoints(renderer, points, 360) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderPoints error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawText(const char* text, int x, int y, SDL_Color color, bool useCamera) const
{
	if (text == nullptr || text[0] == '\0') return true;

	int scale = Engine::GetInstance().window->GetScale();

	// Color del texto (SDL_RenderDebugText usa el color actual del renderer)
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	float X = 0.0f;
	float Y = 0.0f;

	if (useCamera)
	{
		X = (float)(camera.x + x * scale);
		Y = (float)(camera.y + y * scale);
	}
	else
	{
		X = (float)(x * scale);
		Y = (float)(y * scale);
	}

	// Texto debug de SDL3 (perfecto para contadores simples)
	bool ok = SDL_RenderDebugText(renderer, X, Y, text);
	if (!ok)
	{
		LOG("SDL_RenderDebugText error: %s", SDL_GetError());
	}
	return ok;
}

bool Render::DrawTextureScaled(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float scaleFactor, bool useCamera) const
{
	if (texture == nullptr) return false;

	int scale = Engine::GetInstance().window->GetScale();

	// --- Convert SRC rect to SDL_FRect (SDL3 expects floats) ---
	SDL_FRect srcF;
	SDL_FRect* pSrc = nullptr;

	int w = 0, h = 0;

	if (section != nullptr)
	{
		srcF.x = (float)section->x;
		srcF.y = (float)section->y;
		srcF.w = (float)section->w;
		srcF.h = (float)section->h;
		pSrc = &srcF;

		w = section->w;
		h = section->h;
	}
	else
	{
		// If no section, we need texture size to build dst w/h
		float tw = 0.0f, th = 0.0f;
		SDL_GetTextureSize(texture, &tw, &th);
		w = (int)tw;
		h = (int)th;
	}

	// --- DEST rect in screen coords ---
	float drawX = 0.0f;
	float drawY = 0.0f;

	if (useCamera)
	{
		drawX = (float)(camera.x + x * scale);
		drawY = (float)(camera.y + y * scale);
	}
	else
	{
		drawX = (float)(x * scale);
		drawY = (float)(y * scale);
	}

	SDL_FRect dstF;
	dstF.x = drawX;
	dstF.y = drawY;
	dstF.w = (float)(w * scale * scaleFactor);
	dstF.h = (float)(h * scale * scaleFactor);

	return SDL_RenderTexture(renderer, texture, pSrc, &dstF);
}