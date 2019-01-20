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
#include <cmath>
#include <math.h>

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
		vector<vector<Direction>> get_all_permutations(int move_number) const
		{
			vector<Direction> directions = { Direction::STILL, Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST };

			vector<vector<Direction>> all_path_permutations((int)pow(5, move_number));

			if (move_number == 2)
			{
				int i = 0;
				for (Direction direction1 : directions)
					for (Direction direction2 : directions)
						all_path_permutations[i++] = { direction1, direction2 };
			}
			else if (move_number == 3)
			{
				int i = 0;
				for (Direction direction1 : directions)
					for (Direction direction2 : directions)
						for (Direction direction3 : directions)
							all_path_permutations[i++] = { direction1, direction2, direction3 };
			}
			else if (move_number == 4)
			{
				int i = 0;
				for (Direction direction1 : directions)
					for (Direction direction2 : directions)
						for (Direction direction3 : directions)
							for (Direction direction4 : directions)
								all_path_permutations[i++] = { direction1, direction2, direction3, direction4 };
			}
			else if (move_number == 5)
			{
				int i = 0;
				for (Direction direction1 : directions)
					for (Direction direction2 : directions)
						for (Direction direction3 : directions)
							for (Direction direction4 : directions)
								for (Direction direction5 : directions)
									all_path_permutations[i++] = { direction1, direction2, direction3, direction4, direction5 };
			}
			else if (move_number == 6)
			{
				int i = 0;
				for (Direction direction1 : directions)
					for (Direction direction2 : directions)
						for (Direction direction3 : directions)
							for (Direction direction4 : directions)
								for (Direction direction5 : directions)
									for (Direction direction6 : directions)
										all_path_permutations[i++] = { direction1, direction2, direction3, direction4, direction5, direction6 };
			}
			else
			{
				log::log("Error: move_number invalid");
				exit(1);
			}

			return all_path_permutations;
		}

		pair<Position, double> find_best_action(shared_ptr<Ship> ship, const Game& game) const;
		pair<Position, double> find_best_extract_move(shared_ptr<Ship> ship, const Game& game, int reach) const;
		double score_path(shared_ptr<Ship> ship, const vector<Direction>& path, const Game& game) const;
	};
}