#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

class Item : public Entity
{
public:

	Item();
	virtual ~Item();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	bool Destroy();

public:

	bool isPicked = false;

private:

	SDL_Texture* textureDaga;
	const char* texturePath;
	int texW, texH;

	PhysBody* pbody;
};
