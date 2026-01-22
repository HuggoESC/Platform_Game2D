#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

class Textures : public Module
{
public:

	Textures();

	virtual ~Textures();

	bool Awake();

	bool Start();

	bool CleanUp();

	SDL_Texture* const Load(const char* path);
	SDL_Texture* const LoadSurface(SDL_Surface* surface);
	bool UnLoad(SDL_Texture* texture);
	void GetSize(const SDL_Texture* texture, int& width, int& height) const;

public:
	std::list<SDL_Texture*> textures;

};
