#pragma once

#include "position.hpp"
#include "direction.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "command.hpp"
#include "map_cell.hpp"
#include "priority_queue.hpp"

#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <utility>

using namespace std;

namespace hlt
{
	struct Game;
	class Blocker
	{
	public:
		unordered_map<Position, unordered_map<Position, double>> positions_to_block_scores;

		Blocker() {};

		void fill_positions_to_block_scores(const Game& game);
		unordered_map<Position, double> position_to_block_on_enemy_base(const Position& enemy_base, const Game& game) const;

		Objective find_best_objective_cell(shared_ptr<Ship> ship, const Game& game) const;
		void decrease_score_in_position(const Position& position, const Game& game);
	};
}