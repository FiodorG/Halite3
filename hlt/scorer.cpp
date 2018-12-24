#include "scorer.hpp"
#include "game.hpp"

using namespace hlt;
using namespace std;

void hlt::Scorer::update_grid_score_inspiration(const Game& game)
{
	Stopwatch s("Updating grid_score_inspiration");

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
void hlt::Scorer::update_grid_score_move(const Game& game)
{
	Stopwatch s("Updating grid_score_move");

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
void hlt::Scorer::update_grid_score_enemies(const Game& game)
{
	Stopwatch s("Updating grid_score_enemies");

	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			grid_score_enemies[i][j] = 0;

			// If enemy is in contiguous cell, 9
			if (
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::NORTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::SOUTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::EAST)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::WEST))
				)
				grid_score_enemies[i][j] = 9;

			// if enemy on cell, 10
			if (game.game_map->cells[i][j].is_occupied_by_enemy(game.my_id))
				grid_score_enemies[i][j] = 10;
		}
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

void hlt::Scorer::update_grid_score_dropoff(const Game& game)
{
	Stopwatch s("Updating grid_score_dropoff");

	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = 7;

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
					{
						grid_score_dropoff[i][j] += halite;
					}
				}

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
	Stopwatch s("Updating grid_score_extract");

	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = game.get_constant("Score: Smoothing radius");
	double halite_multiplier = game.is_two_player_game()? 2.0 : 3.0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
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
						halite *= halite_multiplier;

					int distance = game.distance(Position(i, j), Position(new_k, new_l));
					if (distance <= radius)
						grid_score_extract_smooth[i][j] += halite / max((double)distance, 1.0);
				}

			// Any structure has 0 score
			if (game.mapcell(Position(j, i))->has_structure())
				grid_score_extract_smooth[i][j] = 0.0;

			if (false && game.is_four_player_game() && (game.game_map->width <= 40))
			{
				int distance_inf_from_center = game.game_map->calculate_distance_from_axis(Position(j, i), Position(width / 2, height / 2));
				int distance_inf_from_corner = game.game_map->calculate_distance_from_axis(Position(j, i), Position(0, 0));
				int distance_from_center = game.distance(Position(j, i), Position(width / 2, height / 2));
				int distance_from_corner = game.distance(Position(j, i), Position(0, 0));
				int radius = width / 8; // 4 for 32, 5 for 40
				double multiplier = 1.5;

				if (distance_inf_from_center < radius)
					grid_score_extract_smooth[i][j] *= linear_decrease(distance_inf_from_center, 0, radius, 1.0, multiplier);

				if (distance_inf_from_corner < radius)
					grid_score_extract_smooth[i][j] *= linear_decrease(distance_inf_from_corner, 0, radius, 1.0, multiplier);

				if (distance_from_center < radius)
					grid_score_extract_smooth[i][j] *= linear_decrease(distance_from_center, 0, radius, 1.0, multiplier);

				if (distance_from_corner < radius)
					grid_score_extract_smooth[i][j] *= linear_decrease(distance_from_corner, 0, radius, 1.0, multiplier);
			}

			if (game.is_four_player_game() && (game.game_map->width <= 40))
			{
				int distance_inf_from_shipyard = game.game_map->calculate_distance_inf(Position(j, i), game.my_shipyard_position());
				int base_to_axis = width / 4;
				int margin = width / 8 - 1;
				double multiplier = 1.5;

				grid_score_extract_smooth[i][j] *= strangle(
					distance_inf_from_shipyard,
					base_to_axis - margin,
					base_to_axis - margin + 2,
					base_to_axis + margin - 2,
					base_to_axis + margin,
					1.0,
					multiplier,
					multiplier,
					0.9
				);

				if (game.close_to_crowded_area(Position(j, i), margin + 1))
					grid_score_extract_smooth[i][j] *= multiplier;
			}

			grid_score_extract[i][j] = (double)game.mapcell(i, j)->halite;
		}

	//log::log_vectorvector(grid_score_extract);
	//log::log_vectorvector(grid_score_extract_smooth);
}
void hlt::Scorer::update_grid_score_targets(const Game& game)
{
	Stopwatch s("Updating grid_score_targets");

	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = 5;

	grid_score_ships_nearby.clear();
	for (auto& player : game.players)
		grid_score_ships_nearby[player->id] = vector<vector<double>>(height, vector<double>(width, 0.0));

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			Position position = Position(j, i);

			// Enemies and allies around
			for (int k = 0; k <= radius * 2; ++k)
				for (int l = 0; l <= radius * 2; ++l)
				{
					int new_k = (((i - radius + k) % width) + width) % width;
					int new_l = (((j - radius + l) % height) + height) % height;

					Position current_position = Position(new_l, new_k);
					int distance = game.distance(position, current_position);

					if (distance > radius)
						continue;

					if (game.position_has_ship(current_position))
					{
						PlayerId playerid = game.ship_on_position(current_position)->owner;
						grid_score_ships_nearby[playerid][i][j] += max(900.0 - (double)game.mapcell(current_position)->ship->halite, 0.0) / max(1.0, (double)distance);
					}
				}
		}

	//for (auto& player : game.players)
	//{
	//	log::log(to_string(player->id));
	//	log::log_vectorvector(grid_score_ships_nearby[player->id]);
	//}
}
void hlt::Scorer::update_grid_score_can_stay_still(const Game& game)
{
	Stopwatch s("Updating grid_score_can_stay_still");

	int width = game.game_map->width;
	int height = game.game_map->height;
	double score_bump = game.is_two_player_game() ? -500.0 : 0.0;

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			// Initialize to good score
			grid_score_can_stay_still[i][j] = -999.0;
			Position position = Position(j, i);

			if (!game.ally_in_cell(position))
				continue;

			if (game.is_four_player_game() || game.is_two_player_game())
			{
				if (grid_score_move[i][j] < 3)
					grid_score_can_stay_still[i][j] = 1.0;
				else
					grid_score_can_stay_still[i][j] = -1.0;
			}

			if (game.is_two_player_game() || game.is_four_player_game())
			{
				// Find adjacent enemies
				unordered_map<shared_ptr<Ship>, double> adjacent_enemies;
				for (int k = 0; k <= 2; ++k)
					for (int l = 0; l <= 2; ++l)
					{
						int new_k = (((i - 1 + k) % width) + width) % width;
						int new_l = (((j - 1 + l) % height) + height) % height;
						Position enemy_position = Position(new_l, new_k);

						if (game.enemy_in_cell(enemy_position) && (game.distance(position, enemy_position) <= 1)) 
							adjacent_enemies[game.mapcell(enemy_position)->ship] = 0.0;
					}

				// Fill score for attack from each adjacent enemy
				for (auto& enemy_ship : adjacent_enemies)
					adjacent_enemies[enemy_ship.first] = combat_score(game.ship_on_position(position), enemy_ship.first, position, game);

				// fill worst score
				double worst_score = 99999999.0;
				for (auto& enemy_ship : adjacent_enemies)
					if (enemy_ship.second < worst_score)
						worst_score = enemy_ship.second;

				grid_score_can_stay_still[i][j] = worst_score + score_bump;
			}
		}

	//log::log_vectorvector(grid_score_can_stay_still);
}

double Scorer::get_score_ship_move_to_position(shared_ptr<Ship> ship, const Position& position, const Game& game) const
{
	// for now proxy desirability to retreat to cell by scoring with the cell with most ships around

	double score;
	for (auto& grid_it : grid_score_ships_nearby)
		if (grid_it.first != game.my_id)
			score = max(score, grid_it.second[position.y][position.x]);

	return score;
}

double Scorer::combat_score(shared_ptr<Ship> my_ship, shared_ptr<Ship> enemy_ship, const Position& position_to_score, const Game& game) const
{
	double halite_ally = (double)my_ship->halite;
	double halite_enemy = (double)enemy_ship->halite;
	double halite_cell = (double)game.mapcell(position_to_score)->halite;
	double halite_total = (halite_ally + halite_enemy + halite_cell);
	//int inspiration = grid_score_inspiration[i][j];

	double score_attack_allies_nearby = max(get_grid_score_ships_nearby(my_ship->owner, position_to_score) - max(900.0 - halite_ally, 0.0), 0.0);
	double score_attack_enemies_nearby = max(get_grid_score_ships_nearby(enemy_ship->owner, position_to_score) - max(900.0 - halite_enemy, 0.0), 0.0);

	double proba_of_me_getting_back;
	if (score_attack_allies_nearby + score_attack_enemies_nearby > 0.0)
		proba_of_me_getting_back = score_attack_allies_nearby / (score_attack_allies_nearby + score_attack_enemies_nearby);
	else
		proba_of_me_getting_back = 0.0; // don't attack if unclear if can get back

	double score_ally = -halite_ally + halite_total * proba_of_me_getting_back;
	double score_enemy = -halite_enemy + halite_total * (1.0 - proba_of_me_getting_back);

	return score_ally - score_enemy;
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
	bool can_attack = (ship->halite < 500);

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
				can_attack &&
				(grid_score_move[i][j] == 10) &&
				!game.ship_on_position(position)->is_targeted
				)
			{
				double score_combat = combat_score(ship, game.ship_on_position(position), position, game);
				double total_score_attack = 5.0 * max(score_combat, 0.0) / max(1.0, (double)distance_cell_ship);

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

	double remove_multiplier = game.is_two_player_game()? 5 : 15;

	double halite_to_decrease = max((double)ship->missing_halite(), 300.0) / (double)area * remove_multiplier;
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

void hlt::Scorer::update_grid_ship_can_move_to_dangerous_cell(const Game& game)
{
	Stopwatch s("Updating grid_ship_can_move_to_dangerous_cell");

	grid_ship_can_move_to_dangerous_cell.clear();

	for (const shared_ptr<Ship>& my_ship : game.me->my_ships)
	{
		grid_ship_can_move_to_dangerous_cell[my_ship] = unordered_map<Position, double>();

		vector<Position> dangerous_positions = game.nearby_positions_to_position(my_ship->position, 4);

		for (auto& dangerous_position : dangerous_positions)
		{
			vector<shared_ptr<Ship>> enemies = game.enemies_adjacent_to_position(dangerous_position);

			double score = 9999999.0;
			for (auto& enemy_ship : enemies)
				score = min(score, combat_score(my_ship, enemy_ship, dangerous_position, game));

			grid_ship_can_move_to_dangerous_cell[my_ship][dangerous_position] = score;
		}
	}

	//for (auto& ship_it : grid_ship_can_move_to_dangerous_cell)
	//	for (auto& position_it : ship_it.second)
	//		//if (position_it.second != 9999999.0)
	//			log::log(ship_it.first->to_string_ship() + " move to " + position_it.first.to_string_position() + " with score " + to_string(position_it.second));
}