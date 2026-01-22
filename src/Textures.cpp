#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Log.h"

Textures::Textures() : Module()
{
	name = "textures";
}

Textures::~Textures()
{
}

bool Textures::Awake()
{
	LOG("Init Image library");
	bool ret = true;

	return ret;
}

bool Textures::Start()
{
	LOG("start textures");
	bool ret = true;
	return ret;
}

bool Textures::CleanUp()
{
	LOG("Freeing textures and Image library");
	for (const auto& texture : textures) {
		SDL_DestroyTexture(texture);
	}
	textures.clear();

	return true;
}

SDL_Texture* const Textures::Load(const char* path)
{
	SDL_Texture* texture = NULL;
	SDL_Surface* surface = IMG_Load(path);

	if (surface == NULL)
	{
		LOG("Could not load surface with path: %s. IMG_Load: %s", path, SDL_GetError());
	}
	else
	{
		texture = LoadSurface(surface);
		SDL_DestroySurface(surface);
	}

	return texture;
}

bool Textures::UnLoad(SDL_Texture* texture)
{
	for (const auto& _texture : textures) {
		if (_texture == texture) {
			SDL_DestroyTexture(texture);
			return true;
		}
	}
	return false;
}

SDL_Texture* const Textures::LoadSurface(SDL_Surface* surface)
{
	SDL_Texture* texture = SDL_CreateTextureFromSurface(Engine::GetInstance().render->renderer, surface);

	if (texture == NULL)
	{
		LOG("Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		textures.push_back(texture);
	}

	return texture;
}

void Textures::GetSize(const SDL_Texture* texture, int& width, int& height) const
{
	float tw = 0.0f;
	float th = 0.0f;
	if (!SDL_GetTextureSize((SDL_Texture*)texture, &tw, &th))
	{
		LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
		width = 0;
		height = 0;
	}
	else
	{
		width = (int)tw;
		height = (int)th;
	}
}
