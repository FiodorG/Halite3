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
		vector<Objective> objectives_dropoffs;
		int turn_since_last_dropoff;

		ObjectiveManager() : turn_since_last_dropoff(0) {}

		void assign_objectives(Game& game);
		void get_ordered_ships(Game& game);
		void flush_objectives();

		// Dropoffs
		bool should_spawn_dropoff(const Game& game);
		bool can_spawn_dropoff(const shared_ptr<Ship> ship, Game& game);
		void create_dropoff_objective(const Game& game);
	};
}