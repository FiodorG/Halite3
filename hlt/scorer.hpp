#pragma once

#include "ship.hpp"
#include "position.hpp"

#include <vector>

using namespace std;

namespace hlt
{
	struct Game;
	class Scorer
	{
	public:
		vector<vector<int>> grid_score;
		int total_halite;

		Scorer() {};
		
		void add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position);
		void flush_grid_score(const Position& position);
		int get_grid_score(const Position& position) const { return grid_score[position.y][position.x]; }
		void update_grid_score(const Game& game);
	};
}