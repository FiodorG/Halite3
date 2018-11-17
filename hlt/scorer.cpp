#include "scorer.hpp"
#include "game.hpp"

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
double Scorer::butterfly(double x, double x_min, double x_mid, double x_max, double y_min, double y_mid, double y_max) const
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

			// Shipyard should be accessible all the time.
			if (game.is_shipyard_or_dropoff(Position(j, i)))
				grid_score_move[i][j] = 0;
		}
}
void hlt::Scorer::update_grid_score_highway(const Game& game)
{
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			grid_score_highway[i][j] = 1.0;

			// If enemy is in contiguous cell, bad score.
			for (Position& position : game.my_shipyard_or_dropoff_positions())
			{
				int distance = game.game_map->calculate_distance_from_axis(game.game_map->cells[i][j].position, position);

				if (distance == 0)
					grid_score_highway[i][j] = 0.0;
				else if (distance == 1)
					grid_score_highway[i][j] = 0.5;
				else
					grid_score_highway[i][j] = 1.0;
			}
		}
}
void hlt::Scorer::update_grid_score_inspiration(const Game& game)
{
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
			grid_score_inspiration[i][j] = 0;

	int radius = 4;

	for (const auto& player : game.players)
	{
		if (player->id == game.my_id)
			continue;

		for (auto& ship_iterator : player->ships)
		{
			for (int i = 0; i < game.game_map->height; ++i)
				for (int j = 0; j < game.game_map->width; ++j)
				{
					int distance = game.distance(ship_iterator.second->position, Position(j, i));
					if (distance <= radius)
						grid_score_inspiration[i][j] += 1;
				}
		}
	}

	//log::log_vectorvector(grid_score_inspiration);
}
void hlt::Scorer::add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position)
{
	if (ship->is_objective(Objective_Type::BACK_TO_BASE))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::EXTRACT_ZONE))
		grid_score_move[position.y][position.x] = 1;
	else if (ship->is_objective(Objective_Type::EXTRACT))
		grid_score_move[position.y][position.x] = 1;
	else
	{
		log::log("ship has unknown Objective");
		exit(1);
	}
}
void hlt::Scorer::flush_grid_score(const Position& position)
{
	grid_score_move[position.y][position.x] = 0;
}

void hlt::Scorer::update_grid_score_dropoff(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = game.get_constant("Dropoff: No Go Zone");

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Initialize to 0
			grid_score_dropoff[i][j] = 0.0;

			// Halite around adds to score
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;
					double halite = (double)game.mapcell(new_k, new_l)->halite;

					int distance = game.distance(Position(i, j), Position(new_k, new_l));
					if (distance <= radius)
						grid_score_dropoff[i][j] += halite;
				}

			// Any structure has 0 score
			if (game.distance(game.get_closest_enemy_shipyard_or_dropoff(Position(j, i)), Position(j, i)) <= 2)
				grid_score_dropoff[i][j] = 0.0;

			if (game.mapcell(Position(j, i))->has_structure())
				grid_score_dropoff[i][j] = 0.0;
		}

	//log::log_vectorvector(grid_score_dropoff);
}
void hlt::Scorer::update_grid_score_extract(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = game.get_constant("Score: Smoothing radius");

	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			// Initialize to 0
			grid_score_extract_smooth[i][j] = 0.0;

			// Halite around adds to score
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;
					double halite = (double)game.mapcell(new_k, new_l)->halite;

					// Weight down small halite cells
					if (halite < 100)
						halite = halite * halite / 100.0;

					// Add bonus for inspiration
					if (grid_score_inspiration[new_k][new_l] >= 2)
						halite *= (double)game.get_constant("Score: Inspiration Bonus");

					int distance = game.distance(Position(i, j), Position(new_k, new_l));
					if (distance <= radius)
						grid_score_extract_smooth[i][j] += halite / max((double)distance, 1.0);
				}

			// Any structure has 0 score
			if (game.mapcell(Position(j, i))->has_structure())
				grid_score_extract_smooth[i][j] = 0.0;

			grid_score_extract[i][j] = (double)game.mapcell(i, j)->halite;
		}

	//log::log_vectorvector(grid_score_extract);
	//log::log_vectorvector(grid_score_extract_smooth);
}

pair<MapCell*,double> hlt::Scorer::find_best_objective_cell(shared_ptr<Ship> ship, const Game& game, bool verbose) const
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	vector<vector<double>> total_score = vector<vector<double>>(height, vector<double>(width, 0.0));

	double max_score = -DBL_MAX;
	int max_i = 0, max_j = 0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			double halite = grid_score_extract_smooth[i][j];
			double distance_cell_ship = (double)game.distance(ship->position, Position(j, i));
			double distance_cell_shipyard = (double)game.distance(game.get_closest_shipyard_or_dropoff(Position(j, i)), Position(j, i));

			total_score[i][j] = halite / (double)pow(1 + distance_cell_ship + distance_cell_shipyard, 1);

			// Cannot go to objectives further than turns remaining
			if ((int)(1.5 * (distance_cell_ship + distance_cell_shipyard)) >= game.turns_remaining())
				total_score[i][j] = -DBL_MAX;

			if (total_score[i][j] > max_score)
			{
				max_score = total_score[i][j];
				max_i = i;
				max_j = j;
			}
		}

	if (verbose)
	{
		log::log(ship->to_string_ship());

		log::log("Grid Score Extract");
		log::log_vectorvector(grid_score_extract_smooth);

		log::log("Total Score");
		log::log_vectorvector(total_score);
	}

	return make_pair(game.mapcell(max_i, max_j), max_score);
}

pair<MapCell*, double> hlt::Scorer::find_best_dropoff_cell(shared_ptr<Shipyard> shipyard, vector<Position> dropoffs, const Game& game) const
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	vector<vector<double>> total_score = vector<vector<double>>(height, vector<double>(width, 0.0));

	double max_score = -999999.0;
	int max_i = 0, max_j = 0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Find distance to closest shipyards
			int distance = game.distance(game.my_shipyard_position(), Position(j, i));

			for (Position& shipyard_or_dropoff : dropoffs)
				distance = min(game.distance(shipyard_or_dropoff, Position(j, i)), distance);
			
			total_score[i][j] = grid_score_dropoff[i][j] * butterfly(distance, game.get_constant("Dropoff: No Go Zone"), 16, 32, 0.0, 1.0, 0.0);

			if (total_score[i][j] > max_score)
			{
				max_score = total_score[i][j];
				max_i = i;
				max_j = j;
			}
		}

	//log::log("Dropoff Score");
	//log::log_vectorvector(grid_score_dropoff);

	//log::log("Total Score");
	//log::log_vectorvector(total_score);

	return make_pair(game.mapcell(max_i, max_j), grid_score_dropoff[max_i][max_j]);
}

void hlt::Scorer::decreases_score_in_target_area(shared_ptr<Ship> ship, MapCell* target_cell, int radius, const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	int target_x = target_cell->position.x;
	int target_y = target_cell->position.y;

	int area = 2 * radius * radius + 2 * radius + 1;
	double halite_to_decrease = (double)ship->missing_halite() / (double)area * (double)game.get_constant("Score: Remove Halite Multiplier");

	for (int i = 0; i <= radius * 2; ++i)
		for (int j = 0; j <= radius * 2; ++j)
		{
			int new_x = (((target_x - radius + i) % width) + width) % width;
			int new_y = (((target_y - radius + j) % height) + height) % height;

			// When ship assigned to an area, remove missing cargo from the zone's score in radius around.
			if (game.distance(target_cell->position, Position(new_x, new_y)) <= radius)
				grid_score_extract_smooth[new_y][new_x] -= halite_to_decrease;

			grid_score_extract_smooth[new_y][new_x] = max(0.0, grid_score_extract_smooth[new_y][new_x]);
		}

	//log::log("Grid Score Extract");
	//log::log_vectorvector(grid_score_extract);
}

void hlt::Scorer::decreases_score_in_target_cell(shared_ptr<Ship> ship, MapCell* target_cell, double mult, const Game& game)
{
	grid_score_extract_smooth[target_cell->position.y][target_cell->position.x] *= mult;
}

