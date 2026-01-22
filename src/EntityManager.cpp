#include "EntityManager.h"
#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Scene.h"
#include "Log.h"
#include "Gem.h"
#include "Item.h"
#include "Enemy.h"
#include "hoguera.h"

EntityManager::EntityManager() : Module()
{
	name = "entitymanager";
}

EntityManager::~EntityManager()
{}

bool EntityManager::Awake()
{
	LOG("Loading Entity Manager");
	bool ret = true;

	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->Awake();
	}

	return ret;

}

bool EntityManager::Start() {

	bool ret = true; 

	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->Start();
	}

	return ret;
}

bool EntityManager::CleanUp()
{
	bool ret = true;

	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->CleanUp();
	}

	entitiesToDestroy.clear();
	entities.clear();

	return ret;
}

std::shared_ptr<Entity> EntityManager::CreateEntity(EntityType type)
{
	std::shared_ptr<Entity> entity = nullptr;

	switch (type)
	{
	case EntityType::PLAYER:
		entity = std::make_shared<Player>();
		break;
	case EntityType::ITEM:
		entity = std::make_shared<Item>();
		break;
	case EntityType::ENEMY:
		entity = std::make_shared<Enemy>(0, 0);
		break;
	case EntityType::HOGUERA:
		entity = std::make_shared<hoguera>(0, 0);
		break;
	case EntityType::GEM:
		entity = std::make_shared<Gem>(0, 0);
		break;
	case EntityType::LIFEUP: 
	{
		auto e = std::make_shared<LifeUP>(0, 0);
		entities.push_back(e);
		return e;
	}
	default:
		break;
	}

	if (entity != nullptr)
	entities.push_back(entity);

	return entity;
}

void EntityManager::DestroyEntity(std::shared_ptr<Entity> entity)
{
	if (!entity) return;

	for (const auto& e : entitiesToDestroy)
		if (e == entity) return;

	entity->active = false;

	entitiesToDestroy.push_back(entity);
}

void EntityManager::FlushDestroyQueue()
{
	for (auto& e : entitiesToDestroy)
	{
		if (!e) continue;

		e->CleanUp();
		entities.remove(e);
	}
	entitiesToDestroy.clear();
}

void EntityManager::AddEntity(std::shared_ptr<Entity> entity)
{
	if ( entity != nullptr) entities.push_back(entity);

}

bool EntityManager::Update(float dt)
{
	bool ret = true;

	for (const auto& entity : entities)
	{
		if (!entity || entity->active == false) continue;
		ret = entity->Update(dt);
	}

	FlushDestroyQueue();

	return ret;
}