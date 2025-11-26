#pragma once

#include "Entity.h"
#include "Animation.h"
#include "SDL3/SDL.h"
#include <box2d/box2d.h>
#include "Physics.h"	

class Enemy : public Entity
{
public:
    Enemy(int x, int y);
    ~Enemy();

	bool Update(float dt) override; // dt en milisegundos
	void OnCollision(PhysBody* physA, PhysBody* physB) override; // dt en milisegundos

private:
	AnimationSet animations;		// Conjunto de animaciones del enemigo
	SDL_Texture* texture = nullptr; // Textura del enemigo
	PhysBody* pbody = nullptr;   // Cuerpo físico del enemigo
	PhysBody* sensorFront = nullptr; // Sensor frontal
	PhysBody* sensorBack = nullptr;  // Sensor trasero
	float sensorOffset = 16.0f; // Distancia del sensor al centro del enemigo 

	float speed = 1.5f; // pixels por segundo
	int direction = 1; // 1: derecha, -1: izquierda

};