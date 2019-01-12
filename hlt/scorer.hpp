#pragma once

#include "ship.hpp"
#include "shipyard.hpp"
#include "map_cell.hpp"
#include "position.hpp"
#include "stopwatch.hpp"

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
		vector<vector<double>> grid_score_enemies;

		vector<vector<double>> grid_score_extract;
		vector<vector<double>> grid_score_dropoff;
		vector<vector<double>> grid_score_extract_smooth;
		vector<vector<double>> grid_score_neighbor_cell;

		vector<vector<int>> grid_score_inspiration;
		vector<vector<int>> grid_score_inspiration_enemies;
		vector<vector<int>> grid_score_enemies_distance_2;

		unordered_map<PlayerId, vector<vector<double>>> grid_score_ships_nearby;
		unordered_map<shared_ptr<Ship>, unordered_map<Position, double>> grid_ship_can_move_to_dangerous_cell;

		vector<vector<double>> grid_score_can_stay_still;
		vector<vector<int>> grid_score_allies_around;

		int halite_initial;
		int halite_total;
		int halite_percentile;

		Scorer() : halite_initial(0), halite_total(0), halite_percentile(0) {};
		Scorer(int height, int width) : halite_initial(0), halite_total(0), halite_percentile(0) 
		{
			grid_score_move = vector<vector<int>>(height, vector<int>(width, 0));
			grid_score_enemies = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_ships_nearby = unordered_map<PlayerId, vector<vector<double>>>();

			grid_score_extract = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_extract_smooth = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_neighbor_cell = vector<vector<double>>(height, vector<double>(width, 0.0));

			grid_score_dropoff = vector<vector<double>>(height, vector<double>(width, 0.0));
			grid_score_inspiration = vector<vector<int>>(height, vector<int>(width, 0));
			grid_score_inspiration_enemies = vector<vector<int>>(height, vector<int>(width, 0));
			grid_score_enemies_distance_2 = vector<vector<int>>(height, vector<int>(width, 0));

			grid_score_can_stay_still = vector<vector<double>>(height, vector<double>(width, 0.0));
		};
		
		void update_grids(const Game& game)
		{
			Stopwatch s("Updating grids");
			update_grid_score_move(game); // no dep
			//update_grid_score_enemies(game); // no dep
			update_grid_score_inspiration(game); // no dep
			update_grid_score_neighbor_cell(game); // no dep
			update_grid_score_extract(game); // depend on grid_score_inspiration
			update_grid_score_dropoff(game); // depend on grid_score_move
			update_grid_score_targets(game); // no dep
			update_grid_score_can_stay_still(game); // depend on grid_score_move, grid_score_targets
			update_grid_ship_can_move_to_dangerous_cell(game); // depend on grid_score_move, grid_score_targets
		}

		// Move scorer
		void add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position);
		inline void flush_grid_score(const Position& position) { grid_score_move[position.y][position.x] = 0; }
		inline int get_grid_score_move(const Position& position) const { return grid_score_move[position.y][position.x]; }
		inline double get_grid_score_enemies(const Position& position) const { return grid_score_enemies[position.y][position.x]; }
		void update_grid_score_move(const Game& game);
		void update_grid_score_enemies(const Game& game);

		// Strategic extraction scorer
		void update_grid_score_inspiration(const Game& game);
		void update_grid_score_extract(const Game& game);
		void update_grid_score_neighbor_cell(const Game& game);
		void update_grid_score_dropoff(const Game& game);
		inline double get_grid_score_extract(const Position& position) const { return grid_score_extract[position.y][position.x]; }
		inline int get_grid_score_inspiration(const Position& position) const { return grid_score_inspiration[position.y][position.x]; }
		inline int get_grid_score_inspiration_enemies(const Position& position) const { return grid_score_inspiration_enemies[position.y][position.x]; }
		inline double get_grid_score_neighbor_cell(const Position& position) const { return grid_score_neighbor_cell[position.y][position.x]; }

		Objective find_best_objective_cell(shared_ptr<Ship> ship, const Game& game, bool verbose = false) const;
		void decreases_score_in_target_area(shared_ptr<Ship> ship, const Position& position, const Game& game);
		void decreases_score_in_target_cell(shared_ptr<Ship> ship, const Position& position, double mult, const Game& game) { grid_score_extract_smooth[position.y][position.x] *= mult; }

		// Shipyard construction
		pair<MapCell*, double> find_best_dropoff_cell(shared_ptr<Shipyard> shipyard, vector<Position> dropoffs, const Game& game) const;

		// Attack
		void update_grid_score_targets(const Game& game);
		double get_grid_score_ships_nearby(PlayerId id, const Position& position) const { return grid_score_ships_nearby.at(id)[position.y][position.x]; }
		double combat_score(shared_ptr<Ship> my_ship, shared_ptr<Ship> enemy_ship, const Position& position_to_score, const Game& game, bool big_cell = false) const;

		// Can Stay Still and Move
		void update_grid_score_can_stay_still(const Game& game);
		inline double get_grid_score_can_stay_still(const Position& position) const { return grid_score_can_stay_still[position.y][position.x]; }
		double get_score_ship_move_to_position(shared_ptr<Ship> ship, const Position& position, const Game& game) const;
		void update_grid_ship_can_move_to_dangerous_cell(const Game& game);
		inline double get_score_ship_can_move_to_dangerous_cell(shared_ptr<Ship> ship, const Position& position) const 
		{ 
			return grid_ship_can_move_to_dangerous_cell.at(ship).at(position);
		}

		// utilities
		static double linear_increase(int x, int x_min, int x_max, double y_min, double y_max)
		{
			if (x <= x_min)
				return y_min;
			else if (x >= x_max)
				return y_max;
			else
				return y_min + ((double)(x - x_min) / (double)(x_max - x_min)) * (y_max - y_min);
		}
		static double linear_decrease(int x, int x_min, int x_max, double y_min, double y_max)
		{
			if (x <= x_min)
				return y_max;
			else if (x >= x_max)
				return y_min;
			else
				return y_max - ((double)(x - x_min) / (double)(x_max - x_min)) * (y_max - y_min);
		}
		static double butterfly(double x, double x_min, double x_mid, double x_max, double y_min, double y_mid, double y_max)
		{
			if (x <= x_min)
				return y_min;
			else if (x >= x_max)
				return y_max;
			else if ((x >= x_min) && (x <= x_mid))
				return y_min + ((double)(x - x_min) / (double)(x_mid - x_min)) * (y_mid - y_min);
			else //if ((x <= x_max) && (x >= x_mid))
				return y_mid - ((double)(x - x_mid) / (double)(x_max - x_mid)) * (y_mid - y_max);
		}
		static double linear_decrease(double x, double x_min, double x_max, double y_min, double y_max)
		{
			if (x <= x_min)
				return y_max;
			else if (x >= x_max)
				return y_min;
			else
				return y_max - ((x - x_min) / (x_max - x_min)) * (y_max - y_min);
		}
		static double strangle(double x, double x_min, double x_mid1, double x_mid2, double x_max, double y_min, double y_mid1, double y_mid2, double y_max)
		{
			if (x <= x_min)
				return y_min;
			else if (x >= x_max)
				return y_max;
			else if ((x >= x_min) && (x <= x_mid1))
				return y_min + ((double)(x - x_min) / (double)(x_mid1 - x_min)) * (y_mid1 - y_min);
			else if ((x >= x_mid1) && (x <= x_mid2))
				return y_mid1 + ((double)(x - x_mid1) / (double)(x_mid2 - x_mid1)) * (y_mid2 - y_mid1);
			else //if ((x <= x_max) && (x >= x_mid2))
				return y_mid2 - ((double)(x - x_mid2) / (double)(x_max - x_mid2)) * (y_mid2 - y_max);
		}
	};
}