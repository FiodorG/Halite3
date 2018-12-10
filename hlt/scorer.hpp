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

		vector<vector<double>> grid_score_attack_allies_nearby;
		vector<vector<double>> grid_score_attack_enemies_nearby;
		vector<vector<double>> grid_score_attack_allies_nearby_initial;
		vector<vector<double>> grid_score_attack_enemies_nearby_initial;

		vector<vector<double>> grid_score_can_stay_still;
		vector<vector<int>> grid_score_can_move;

		int halite_initial;
		int halite_total;
		int halite_percentile;

		Scorer(int height, int width) : halite_initial(0), halite_total(0), halite_percentile(0) 
		{
			grid_score_move = vector<vector<int>>(height, vector<int>(width, 0));
			grid_score_highway = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_extract = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_dropoff = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_extract_smooth = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_inspiration = vector<vector<int>>(height, vector<int>(width, 0));
			grid_score_attack_allies_nearby = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_attack_enemies_nearby = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_attack_allies_nearby_initial = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_attack_enemies_nearby_initial = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_can_stay_still = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_can_move = vector<vector<int>>(height, vector<int>(width, 0.0));
		};
		
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
		static double linear_decrease(double x, double x_min, double x_max, double y_min, double y_max);
		double butterfly(double x, double x_min, double x_mid, double x_max, double y_min, double y_mid, double y_max) const;

		Objective find_best_objective_cell(shared_ptr<Ship> ship, const Game& game, bool verbose = false) const;
		void decreases_score_in_target_area(shared_ptr<Ship> ship, const Position& position, const Game& game);
		void decreases_score_in_target_cell(shared_ptr<Ship> ship, const Position& position, double mult, const Game& game);

		// Shipyard construction
		pair<MapCell*, double> find_best_dropoff_cell(shared_ptr<Shipyard> shipyard, vector<Position> dropoffs, const Game& game) const;

		// Attack
		void update_grid_score_targets(const Game& game);
		double get_grid_score_allies_nearby(const Position& position) const { return grid_score_attack_allies_nearby[position.y][position.x]; }
		double get_grid_score_attack_enemies_nearby(const Position& position) const { return grid_score_attack_enemies_nearby[position.y][position.x]; }

		// Can Stay Still and Move
		void update_grid_score_can_stay_still(const Game& game);
		void update_grid_score_can_move(const Game& game);
		double get_grid_score_can_stay_still(const Position& position) const { return grid_score_can_stay_still[position.y][position.x]; }
		double get_grid_score_can_move(const Position& position) const { return grid_score_can_move[position.y][position.x]; }
	};
}