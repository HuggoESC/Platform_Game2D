
#include <iostream>
#include "Engine.h"
#include "Log.h"

int main(int argc, char* argv[]) {

	LOG("Engine starting ...");

	Engine::EngineState state = Engine::EngineState::CREATE;
	int result = EXIT_FAILURE;

	while (state != Engine::EngineState::EXIT)
	{
		switch (state)
		{
		case Engine::EngineState::CREATE:
			LOG("CREATION PHASE ===============================");
			state = Engine::EngineState::AWAKE;

			break;

		case Engine::EngineState::AWAKE:
			LOG("AWAKE PHASE ===============================");
			if (Engine::GetInstance().Awake() == true)
				state = Engine::EngineState::START;
			else
			{
				LOG("ERROR: Awake failed");
				state = Engine::EngineState::FAIL;
			}

			break;

		case Engine::EngineState::START:
			LOG("START PHASE ===============================");
			if (Engine::GetInstance().Start() == true )
			{
				state = Engine::EngineState::LOOP;
				LOG("UPDATE PHASE ===============================");
			}
			else
			{
				state = Engine::EngineState::FAIL;
				LOG("ERROR: Start failed");
			}
			break;

		case Engine::EngineState::LOOP:
			if (Engine::GetInstance().Update() == false)
				state = Engine::EngineState::CLEAN;
			break;

		case Engine::EngineState::CLEAN:
			LOG("CLEANUP PHASE ===============================");
			if (Engine::GetInstance().CleanUp() == true)
			{
				result = EXIT_SUCCESS;
				state = Engine::EngineState::EXIT;
			}
			else
				state = Engine::EngineState::FAIL;

			break;

		case Engine::EngineState::FAIL:
			LOG("Exiting with errors");
			result = EXIT_FAILURE;
			state = Engine::EngineState::EXIT;
			break;
		}
	}

	LOG("Closing Engine ===============================");

	return result;
}
