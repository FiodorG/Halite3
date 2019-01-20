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
		EXTRACT_ZONE,
		MOVE,
		ATTACK
	};

	struct Objective
	{
		int id;
		Objective_Type type;
		Position target_position;
		double score;

		Objective() {}
		Objective(int id, Objective_Type type) : id(id), type(type), score(0) {}
		Objective(int id, Objective_Type type, Position target_position) : id(id), type(type), target_position(target_position), score(0) {}
		Objective(int id, Objective_Type type, Position target_position, double score) : id(id), type(type), target_position(target_position), score(score) {}

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
			case MOVE:
				string_type = "MOVE";
				break;
			case EXTRACT_ZONE:
				string_type = "EXTRACT ZONE";
				break;
			case ATTACK:
				string_type = "ATTACK";
				break;
			default:
				break;
			}

			return "Objective(" + string_type + ":" + target_position.to_string_position() + ", " + to_string((int)score) + ")";
		}
	};
}

//namespace std
//{
//	template <> struct hash<hlt::Objective_Type>
//	{
//		size_t operator()(const hlt::Objective_Type& objective_type) const
//		{
//			switch (objective_type)
//			{
//			case EXTRACT:
//				return 1;
//			case BACK_TO_BASE:
//				return 2;
//			case BLOCK_ENEMY_BASE:
//				return 3;
//			case SUICIDE_ON_BASE:
//				return 4;
//			case MAKE_DROPOFF:
//				return 5;
//			case MOVE:
//				return 6;
//			case EXTRACT_ZONE:
//				return 7;
//			default:
//				return 8;
//			}
//		}
//	};
//}