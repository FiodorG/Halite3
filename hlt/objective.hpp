#pragma once

#include "log.hpp"
#include "position.hpp"

#include <ctime>
#include <string>

using namespace std;
using namespace hlt;

namespace hlt 
{
	enum Objective_Type
	{
		EXTRACT,
		BACK_TO_BASE,
		BLOCK_ENEMY_BASE,
		SUICIDE_ON_BASE,
		MAKE_DROPOFF,
	};

	struct Objective
	{
		int id;
		Objective_Type type;
		Position target_position;

		Objective() {}
		Objective(int id, Objective_Type type) : id(id), type(type) {}
		Objective(int id, Objective_Type type, Position target_position) : id(id), type(type), target_position(target_position) {}

		string to_string_objective()
		{
			string string_type;
			switch (type)
			{
			case EXTRACT: 
				string_type = "EXTRACT";
				break;
			case BACK_TO_BASE: 
				string_type = "BACK TO BASE";
				break;
			case BLOCK_ENEMY_BASE:
				string_type = "BLOCK BASE";
				break;
			case SUICIDE_ON_BASE:
				string_type = "SUICIDE ON BASE";
				break;
			case MAKE_DROPOFF:
				string_type = "MAKE DROPOFF";
				break;
			default:
				break;
			}

			return "Objective(" + string_type + ":" + target_position.to_string_position() + ")";
		}
	};
}