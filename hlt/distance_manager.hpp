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
		unordered_map<shared_ptr<Ship>, vector<vector<int>>> ship_distances;
		unordered_map<Position, vector<vector<int>>> shipyard_or_dropoff_distances;

		DistanceManager() {}

		//void fill_distances(const Game& game);
	};
}