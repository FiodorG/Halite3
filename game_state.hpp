#include "objective_manager.hpp"

#include <list>
#include <random>
#include <ctime>

namespace hlt
{
	class GameState
	{
		public:
		std::unique_ptr<Objective_Manager> objective_manager;

		GameState() :
			objective_manager(std::make_unique<Objective_Manager>())
		{
		}
	};
}