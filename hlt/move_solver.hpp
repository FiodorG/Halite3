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
	class MoveSolver
	{
		public:
		vector<vector<Direction>> all_path_permutations;

		MoveSolver() {}
		MoveSolver(int move_number) 
		{
			all_path_permutations = get_all_permutations(move_number);
		}

		vector<shared_ptr<Ship>> filter_ships_without_actions(unordered_map<shared_ptr<Ship>, int> ships_without_actions) const;
		pair<Position, int> find_best_action(shared_ptr<Ship> ship, const Game& game) const;
		pair<Position, int> find_best_extract_move(shared_ptr<Ship> ship, const Game& game) const;
		bool valid_move(const Position& position, const Game& game) const;
		int final_cargo_on_path(shared_ptr<Ship> ship, const vector<Direction>& path, const Game& game) const;
		vector<vector<Direction>> get_all_permutations(int move_number) const;
	};
}