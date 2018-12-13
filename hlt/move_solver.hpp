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
		vector<vector<Direction>> all_path_permutations_2;
		vector<vector<Direction>> all_path_permutations_3;
		vector<vector<Direction>> all_path_permutations_4;
		vector<vector<Direction>> all_path_permutations_5;
		vector<vector<Direction>> all_path_permutations_6;

		MoveSolver() 
		{
			all_path_permutations_2 = get_all_permutations(2);
			all_path_permutations_3 = get_all_permutations(3);
			all_path_permutations_4 = get_all_permutations(4);
			all_path_permutations_5 = get_all_permutations(5);
			all_path_permutations_6 = get_all_permutations(6);
		}

		const vector<vector<Direction>>* get_path_permutations(int reach) const
		{
			switch (reach)
			{
			case 2:
				return &all_path_permutations_2;
			case 3:
				return &all_path_permutations_3;
			case 4:
				return &all_path_permutations_4;
			case 5:
				return &all_path_permutations_5;
			case 6:
				return &all_path_permutations_6;
			default:
				log::log(string("Error: invalid path permuataion."));
				exit(1);
			}
		}
		Direction get_best_direction(int best_score_index, int best_score_move, int reach) const
		{
			switch (reach)
			{
			case 2:
				return all_path_permutations_2[best_score_index][best_score_move];
			case 3:
				return all_path_permutations_3[best_score_index][best_score_move];
			case 4:
				return all_path_permutations_4[best_score_index][best_score_move];
			case 5:
				return all_path_permutations_5[best_score_index][best_score_move];
			case 6:
				return all_path_permutations_6[best_score_index][best_score_move];
			default:
				log::log(string("Error: invalid path permuataion."));
				exit(1);
			}
		}

		pair<Position, double> find_best_action(shared_ptr<Ship> ship, const Game& game) const;

		double score_path(shared_ptr<Ship> ship, const vector<Direction>& path, int reach, int distance_margin, const Game& game) const;
		pair<Position, double> find_best_extract_move(shared_ptr<Ship> ship, const Game& game, int reach, int distance_margin) const;

		bool valid_move(const Position& position, const Game& game) const;
		bool allied_priority_ships(const Position& position, const Game& game) const;
		bool can_stay_still(const Position& position, const Game& game) const;
		vector<vector<Direction>> get_all_permutations(int move_number) const;

		bool is_stuck_ship(shared_ptr<Ship> ship, const Game& game) const;
		Direction best_evade_direction(shared_ptr<Ship> ship, const Game& game) const;
	};
}