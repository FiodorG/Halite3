#pragma once

#include "position.hpp"
#include "ship.hpp"

#include <unordered_map>
#include <utility>
#include <limits>

using namespace std;

namespace hlt
{
	struct Game;
	class ObjectiveManager
	{
		public:
		vector<shared_ptr<Ship>> ships_ordered;
		int turn_since_last_dropoff;

		ObjectiveManager() : turn_since_last_dropoff(0) {}

		void assign_objectives(Game& game);
		void get_ordered_ships(Game& game);
		void flush_objectives();

		// Dropoffs
		bool should_spawn_dropoff(const Game& game, vector<Objective> objectives_dropoffs);
		bool can_spawn_dropoff(const shared_ptr<Ship> ship, Game& game);
		vector<Objective> create_dropoff_objectives(const Game& game);
	};
}