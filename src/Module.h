#pragma once

#include <string>
#include "pugixml.hpp"

class Module
{
public:

	Module() : active(false)
	{}

	void Init()
	{
		active = true;
	}

	virtual bool Awake()
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	virtual bool PreUpdate()
	{
		return true;
	}

	virtual bool Update(float dt)
	{
		return true;
	}

	virtual bool PostUpdate()
	{
		return true;
	}

	virtual bool CleanUp()
	{
		return true;
	}
	
	
	virtual bool LoadParameters(pugi::xml_node parameters)
	{
		configParameters = parameters;
		return true;
	}

	

public:

	std::string name;
	bool active;
	
	pugi::xml_node configParameters;

};