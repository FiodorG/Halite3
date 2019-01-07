#pragma once

#include "position.hpp"
#include "ship.hpp"

#include <unordered_map>
#include <utility>

using namespace std;

namespace hlt
{
	struct Game;
	class DistanceManager
	{
		public:
		vector<vector<Position>> closest_shipyard_or_dropoff;
		vector<vector<int>> distance_cell_shipyard_or_dropoff;

		DistanceManager() {}

		void fill_closest_shipyard_or_dropoff(const Game& game);

		Position get_closest_shipyard_or_dropoff(const Position& position) const { return closest_shipyard_or_dropoff[position.y][position.x]; }
		Position get_closest_shipyard_or_dropoff(shared_ptr<Ship> ship) const { return get_closest_shipyard_or_dropoff(ship->position); }

		inline int get_distance_cell_shipyard_or_dropoff(const Position& position) const { return distance_cell_shipyard_or_dropoff[position.y][position.x]; }
		inline int get_distance_cell_shipyard_or_dropoff(shared_ptr<Ship> ship) const { return get_distance_cell_shipyard_or_dropoff(ship->position); }
	};
}