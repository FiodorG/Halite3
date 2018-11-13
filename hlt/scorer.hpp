#pragma once

#include "ship.hpp"
#include "shipyard.hpp"
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
		vector<vector<double>> grid_score_highway;
		vector<vector<int>> grid_score_move;
		vector<vector<double>> grid_score_extract;
		vector<vector<double>> grid_score_dropoff;
		vector<vector<double>> grid_score_extract_smooth;
		vector<vector<int>> grid_score_inspiration;
		int halite_initial;
		int halite_total;
		int halite_percentile;

		Scorer() : halite_initial(0), halite_total(0), halite_percentile(0) {};
		
		// Move scorer
		void add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position);
		void flush_grid_score(const Position& position);
		int get_grid_score_move(const Position& position) const { return grid_score_move[position.y][position.x]; }
		double get_grid_score_highway(const Position& position) const { return grid_score_highway[position.y][position.x]; }
		void update_grid_score_move(const Game& game);
		void update_grid_score_highway(const Game& game);

		// Strategic extraction scorer
		void update_grid_score_inspiration(const Game& game);
		void update_grid_score_extract(const Game& game);
		void update_grid_score_dropoff(const Game& game);
		double get_grid_score_extract(const Position& position) const { return grid_score_extract[position.y][position.x]; }
		double get_grid_score_inspiration(const Position& position) const { return grid_score_inspiration[position.y][position.x]; }

		static double linear_increase(int x, int x_min, int x_max, double y_min, double y_max);
		static double linear_decrease(int x, int x_min, int x_max, double y_min, double y_max);
		double butterfly(double x, double x_min, double x_mid, double x_max, double y_min, double y_mid, double y_max) const;

		pair<MapCell*, double> find_best_objective_cell(shared_ptr<Ship> ship, const Game& game, bool verbose = false) const;
		void decreases_score_in_target_area(shared_ptr<Ship> ship, MapCell* target_cell, int radius, const Game& game);
		void decreases_score_in_target_cell(shared_ptr<Ship> ship, MapCell* target_cell, double mult, const Game& game);

		// Shipyard construction
		pair<MapCell*, double> find_best_dropoff_cell(shared_ptr<Shipyard> shipyard, vector<Position> dropoffs, const Game& game) const;
	};
}