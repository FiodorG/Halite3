#pragma once

#include "ship.hpp"
#include "map_cell.hpp"
#include "position.hpp"

#include <vector>
#include <utility>

using namespace std;

namespace hlt
{
	struct Game;
	class Scorer
	{
	public:
		vector<vector<int>> grid_score_move;
		vector<vector<double>> grid_score_extract;
		vector<vector<double>> grid_score_extract_smooth;
		int total_halite;

		Scorer() : total_halite(0) {};
		
		// Move scorer
		void add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position);
		void flush_grid_score(const Position& position);
		int get_grid_score_move(const Position& position) const { return grid_score_move[position.y][position.x]; }
		void update_grid_score_move(const Game& game);

		// Strategic extraction scorer
		void update_grid_score_extract(const Game& game);
		double get_grid_score_extract(const Position& position) const { return grid_score_extract[position.y][position.x]; }
		static double linear_increase(int x, int x_min, int x_max, double y_min, double y_max);
		static double linear_decrease(int x, int x_min, int x_max, double y_min, double y_max);

		pair<MapCell*, double> find_best_objective_cell(shared_ptr<Ship> ship, const Game& game) const;
		void decreases_score_in_target_area(shared_ptr<Ship> ship, MapCell* target_cell, const Game& game);
		void decreases_score_in_target_cell(shared_ptr<Ship> ship, MapCell* target_cell, const Game& game);
	};
}