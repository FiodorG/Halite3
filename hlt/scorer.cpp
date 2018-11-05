#include "scorer.hpp"
#include "game.hpp"

#include <cmath>

using namespace hlt;
using namespace std;

double Scorer::linear_increase(int x, int x_min, int x_max, double y_min, double y_max)
{
	if (x <= x_min)
		return y_min;
	else if (x >= x_max)
		return y_max;
	else
		return y_min + ((double)(x - x_min) / (double)(x_max - x_min)) * (y_max - y_min);
}

double Scorer::linear_decrease(int x, int x_min, int x_max, double y_min, double y_max)
{
	if (x <= x_min)
		return y_max;
	else if (x >= x_max)
		return y_min;
	else
		return y_max - ((double)(x - x_min) / (double)(x_max - x_min)) * (y_max - y_min);
}

void hlt::Scorer::update_grid_score_move(const Game& game)
{
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			grid_score_move[i][j] = 0;

			// If enemy is in contiguous cell, bad score.
			if (
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::NORTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::SOUTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::EAST)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::WEST)) ||
				game.game_map->cells[i][j].is_occupied_by_enemy(game.my_id)
				)
				grid_score_move[i][j] = 9;
		}
}

void hlt::Scorer::add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position)
{
	grid_score_move[position.y][position.x] += 1;
}

void hlt::Scorer::flush_grid_score(const Position& position)
{
	grid_score_move[position.y][position.x] = max(0, grid_score_move[position.y][position.x] - 1);
}

void hlt::Scorer::update_grid_score_extract(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
			grid_score_extract_smooth[i][j] = 0;

	int radius = game.get_constant("Score: Smoothing radius");
	int area = 2 * radius * radius + 2 * radius + 1;

	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			// Halite around adds to score
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;

					if (game.distance(Position(i, j), Position(new_k, new_l)) <= radius)
						grid_score_extract_smooth[i][j] += (double)game.mapcell(new_k, new_l)->halite / (double)area;
				}

			grid_score_extract[i][j] = (double)game.mapcell(i, j)->halite;
		}

	//log::log_vectorvector(grid_score_extract);
	//log::log_vectorvector(grid_score_extract_smooth);
}

pair<MapCell*,double> hlt::Scorer::find_best_objective_cell(shared_ptr<Ship> ship, const Game& game) const
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	//pair<vector<vector<int>>, vector<vector<int>>> all_costs = game.pathfinder.dijkstra_costs(game.mapcell(ship), game);
	//vector<vector<int>> move_costs = all_costs.first;
	//vector<vector<int>> move_turns = all_costs.second;
	vector<vector<double>> total_score = vector<vector<double>>(height, vector<double>(width, 0.0));

	double max_score = -999999.0;
	int max_i = 0, max_j = 0;
	double halite = 0.0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			halite = grid_score_extract_smooth[i][j];

			if (halite <= 100.0)
				halite *= halite / 100.0;

			total_score[i][j] =
				// Score of ressources taken cannot be > cargo
				min(halite, 950.0 - (double)ship->halite)
				// Cost in halite of going there
				- linear_decrease(game.turn_number, 0, 400, 0.0, 8.0) * (double)game.distance(ship->position, Position(j, i))
				// Cost in halite of going back to base
				- linear_decrease(game.turn_number, 0, 400, 0.0, 8.0) * (double)game.distance(game.get_closest_shipyard_or_dropoff(ship), Position(j, i));

			if (total_score[i][j] > max_score)
			{
				max_score = total_score[i][j];
				max_i = i;
				max_j = j;
			}
		}

	//if ((game.turn_number == 233))
	//{
	//	log::log(ship->to_string_ship());

	//	log::log("Grid Score Extract");
	//	log::log_vectorvector(grid_score_extract_smooth);

	//	log::log("Total Score");
	//	log::log_vectorvector(total_score);
	//}
	
	//log::log("i:" + to_string(max_i) + ", j:" + to_string(max_j));

	return make_pair(game.mapcell(max_i, max_j), max_score);
}

void hlt::Scorer::decreases_score_in_target_area(shared_ptr<Ship> ship, MapCell* target_cell, const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	int target_x = target_cell->position.x;
	int target_y = target_cell->position.y;

	int radius = game.get_constant("Score: Brute force reach");
	int area = 2 * radius * radius + 2 * radius + 1;

	for (int i = 0; i <= radius * 2; ++i)
		for (int j = 0; j <= radius * 2; ++j)
		{
			int new_x = (((target_x - radius + i) % width) + width) % width;
			int new_y = (((target_y - radius + j) % height) + height) % height;

			// When ship assigned to an area, remove missing cargo from the zone's score in radius around.
			if (game.distance(target_cell->position, Position(new_x, new_y)) <= radius)
				grid_score_extract_smooth[new_y][new_x] -= (950.0 - (double)ship->halite) / (double)area / (double)game.get_constant("Score: Area Discount");

			grid_score_extract_smooth[new_y][new_x] = max(0.0, grid_score_extract_smooth[new_y][new_x]);
		}

	//log::log("Grid Score Extract");
	//log::log_vectorvector(grid_score_extract);
}

void hlt::Scorer::decreases_score_in_target_cell(shared_ptr<Ship> ship, MapCell* target_cell, const Game& game)
{
	//grid_score_extract[target_cell->position.y][target_cell->position.x] -= (1000.0 - (double)ship->halite) * 0.9;
	grid_score_extract[target_cell->position.y][target_cell->position.x] = -999.0;

	//log::log("Grid Score Extract");
	//log::log_vectorvector(grid_score_extract);
}

