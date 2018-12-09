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

double Scorer::linear_decrease(double x, double x_min, double x_max, double y_min, double y_max)
{
	if (x <= x_min)
		return y_max;
	else if (x >= x_max)
		return y_min;
	else
		return y_max - ((x - x_min) / (x_max - x_min)) * (y_max - y_min);
}

void hlt::Scorer::update_grid_score_move(const Game& game)
{
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			grid_score_move[i][j] = 0;

			// If enemy is in contiguous cell, 9
			if (
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::NORTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::SOUTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::EAST)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::WEST))
			)
				grid_score_move[i][j] = 9;

			// if enemy on cell, 10
			if (game.game_map->cells[i][j].is_occupied_by_enemy(game.my_id))
				grid_score_move[i][j] = 10;

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
	else if (ship->is_objective(Objective_Type::ATTACK))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
		grid_score_move[position.y][position.x] = 2;
	else if (ship->is_objective(Objective_Type::BLOCK_ENEMY_BASE))
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
	int radius = 7;
	double enemies_around = 0.0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Initialize to 0
			grid_score_dropoff[i][j] = 0.0;
			enemies_around = 0.0;

			// Halite around adds to score
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;
					double halite = (double)game.mapcell(new_k, new_l)->halite;

					//if (grid_score_inspiration[new_k][new_l] >= 2)
					//	halite *= 2;

					int distance = game.distance(Position(i, j), Position(new_k, new_l));
					if (distance <= radius)
					{
						grid_score_dropoff[i][j] += halite;
						enemies_around += (double)(grid_score_move[new_k][new_l] == 10) / max(1.0, (double)distance);
					}
				}

			// Reduce score if enemies are around
			/*if (enemies_around > 0)
				grid_score_dropoff[i][j] *= linear_decrease(enemies_around, 2.0, 3.0, 0.5, 1.0);*/

			// add more weight in center for 4p games, uniformly in the area.
			if (game.is_four_player_game() && game.close_to_crowded_area(Position(j, i), width / 4))
				grid_score_dropoff[i][j] *= 1.25;

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
void hlt::Scorer::update_grid_score_targets(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = game.get_constant("Score: Attack radius");

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Initialize to bad score
			grid_score_attack_allies_nearby[i][j] = 0.0;
			grid_score_attack_enemies_nearby[i][j] = 0.0;

			Position enemy_position = Position(j, i);
			double number_of_allies = 0.0, number_of_enemies = 0.0;

			// Enemies and allies around
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;

					Position current_position = Position(new_l, new_k);
					int distance = game.distance(enemy_position, current_position);

					if (distance > radius)
						continue;

					if (game.enemy_in_cell(current_position))
						number_of_enemies += (1000.0 - (double)game.mapcell(current_position)->ship->halite) / max(1.0, (double)distance);

					if (game.ally_in_cell(current_position))
						number_of_allies += (1000.0 - (double)game.mapcell(current_position)->ship->halite) / max(1.0, (double)distance);
				}

			grid_score_attack_allies_nearby[i][j] = number_of_allies;
			grid_score_attack_enemies_nearby[i][j] = number_of_enemies;
		}

	//log::log_vectorvector(grid_score_attack_allies_nearby);
	//log::log_vectorvector(grid_score_attack_enemies_nearby);
}
void hlt::Scorer::update_grid_score_can_stay_still(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Initialize to bad score
			grid_score_can_stay_still[i][j] = -1.0;
			Position position = Position(j, i);

			// 4p games can always stay still
			if (game.is_four_player_game())
				grid_score_can_stay_still[i][j] = 1.0;

			// for now, can stay only if no enemies around
			if (game.is_two_player_game())
			{
				if (grid_score_move[i][j] < 5)
					grid_score_can_stay_still[i][j] = 1.0;
				else
					grid_score_can_stay_still[i][j] = 0.0;
			}

			// !!!! make sure to use non decreased grid_score_attack_allies_nearby
			// 2p games can never stay still, for now
			if (false && game.is_two_player_game())
			{
				grid_score_can_stay_still[i][j] = 0.0;

				double halite_ally = (double)game.mapcell(position)->ship->halite;
				double halite_cell = (double)game.mapcell(position)->halite;

				// Find adjacent enemies
				unordered_map<shared_ptr<Ship>, double> adjacent_enemies;
				for (int k = 0; k <= 2; ++k)
					for (int l = 0; l <= 2; ++l)
					{
						int new_k = (((i - 1 + k) % width) + width) % width;
						int new_l = (((j - 1 + l) % height) + height) % height;
						Position enemy_position = Position(new_l, new_k);

						if (game.enemy_in_cell(enemy_position)) 
							adjacent_enemies[game.mapcell(enemy_position)->ship] = 0.0;
					}

				// Fill score for attack from each adjacent enemy
				for (auto& enemy_ship : adjacent_enemies)
				{
					double halite_enemy = (double)enemy_ship.first->halite;

					double score_attack_allies_nearby = max(grid_score_attack_allies_nearby[i][j] - (1000.0 - (double)halite_ally), 0.0);
					double score_attack_enemies_nearby = max(grid_score_attack_enemies_nearby[i][j] - (1000.0 - (double)halite_enemy), 0.0);
					double proba_of_me_getting_back = score_attack_allies_nearby / (score_attack_allies_nearby + score_attack_enemies_nearby);

					double score_ally = -halite_ally + (halite_ally + halite_enemy + halite_cell) * proba_of_me_getting_back;
					double score_enemy = -halite_enemy + (halite_ally + halite_enemy + halite_cell) * (1.0 - proba_of_me_getting_back);

					adjacent_enemies[enemy_ship.first] = score_ally - score_enemy;
				}

				// fill worst score
				double worst_score = DBL_MAX;
				for (auto& enemy_ship : adjacent_enemies)
					if (enemy_ship.second < worst_score)
						worst_score = enemy_ship.second;
				
				grid_score_can_stay_still[i][j] = worst_score;
			}
		}

	//log::log_vectorvector(grid_score_can_stay_still);
}

Objective hlt::Scorer::find_best_objective_cell(shared_ptr<Ship> ship, const Game& game, bool verbose) const
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	//vector<vector<double>> total_score = vector<vector<double>>(height, vector<double>(width, 0.0));

	double max_score = -DBL_MAX;
	int max_i = 0, max_j = 0;
	bool is_two_player_game = game.is_two_player_game();
	int turns_remaining = game.turns_remaining();
	Objective_Type max_type = Objective_Type::EXTRACT_ZONE;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			Objective_Type type = Objective_Type::EXTRACT_ZONE;
			double halite = grid_score_extract_smooth[i][j];
			Position position = Position(j, i);
			int distance_cell_ship = game.distance(ship->position, position);
			int distance_cell_shipyard = game.distance_manager.get_distance_cell_shipyard_or_dropoff(position);

			int total_distance = distance_cell_ship + distance_cell_shipyard;
			double total_score = halite / (1.0 + (double)total_distance);

			// Cannot go to objectives further than turns remaining
			if ((int)(1.5 * total_distance) >= turns_remaining)
				total_score -= 999999.0;

			if (
				is_two_player_game &&
				(grid_score_move[i][j] == 10) && // enemy in cell
				(grid_score_attack_allies_nearby[i][j] > 0.0) // I have more empty halite around
				)
			{
				double halite_ally = (double)ship->halite;
				double halite_enemy = (double)game.mapcell(position)->ship->halite;
				double halite_cell = (double)game.mapcell(position)->halite;
				double halite_total = (halite_ally + halite_enemy + halite_cell);
				//int inspiration = grid_score_inspiration[i][j];

				double score_attack_allies_nearby = max(grid_score_attack_allies_nearby[i][j] - (1000.0 - (double)halite_ally), 0.0);
				double score_attack_enemies_nearby = max(grid_score_attack_enemies_nearby[i][j] - (1000.0 - (double)halite_enemy), 0.0);
				double proba_of_me_getting_back = score_attack_allies_nearby / (score_attack_allies_nearby + score_attack_enemies_nearby);

				double score_ally = -halite_ally + halite_total * proba_of_me_getting_back;
				double score_enemy = -halite_enemy + halite_total * (1.0 - proba_of_me_getting_back);

				double total_score_attack = 10.0 * max(score_ally - score_enemy, 0.0) / max(1.0, (double)distance_cell_ship);

				if (total_score_attack > total_score)
				{
					total_score = total_score_attack;
					type = Objective_Type::ATTACK;
				}
			}

			if (total_score > max_score)
			{
				max_score = total_score;
				max_i = i;
				max_j = j;
				max_type = type;
			}
		}

	//if (verbose)
	//{
	//	log::log(ship->to_string_ship());

	//	log::log("Grid Score Extract");
	//	log::log_vectorvector(grid_score_extract_smooth);

	//	log::log("Total Score");
	//	log::log_vectorvector(total_score);
	//}

	return Objective(-1, max_type, game.mapcell(max_i, max_j)->position, max_score);
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
			
			total_score[i][j] = grid_score_dropoff[i][j] * butterfly(distance, 7, 16, 32, 0.0, 1.0, 0.0);

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

void hlt::Scorer::decreases_score_in_target_area(shared_ptr<Ship> ship, const Position& position, const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;

	int target_x = position.x;
	int target_y = position.y;

	int radius = 4;
	int area = 2 * radius * radius + 2 * radius + 1;
	double halite_to_decrease = max((double)ship->missing_halite(), 300.0) / (double)area * (double)game.get_constant("Score: Remove Halite Multiplier");
	// add max here?

	for (int i = 0; i <= radius * 2; ++i)
		for (int j = 0; j <= radius * 2; ++j)
		{
			int new_x = (((target_x - radius + i) % width) + width) % width;
			int new_y = (((target_y - radius + j) % height) + height) % height;

			// When ship assigned to an area, remove missing cargo from the zone's score in radius around.
			if (game.distance(position, Position(new_x, new_y)) <= radius)
				grid_score_extract_smooth[new_y][new_x] -= halite_to_decrease;

			grid_score_extract_smooth[new_y][new_x] = max(0.0, grid_score_extract_smooth[new_y][new_x]);
		}

	//log::log("Grid Score Extract");
	//log::log_vectorvector(grid_score_extract);
}
void hlt::Scorer::decreases_score_in_target_cell(shared_ptr<Ship> ship, const Position& position, double mult, const Game& game)
{
	grid_score_extract_smooth[position.y][position.x] *= mult;
}