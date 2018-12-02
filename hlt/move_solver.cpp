#include "move_solver.hpp"
#include "game.hpp"

#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace hlt;
using namespace std;


vector<vector<Direction>> MoveSolver::get_all_permutations(int move_number) const
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

bool MoveSolver::valid_move(const Position& position, const Game& game) const
{
	//if (game.is_two_player_game() && (game.scorer.get_grid_score_move(position) > 2))
	//	return true;

	return game.scorer.get_grid_score_move(position) == 0;
}

bool MoveSolver::enemy_at_position(const Position& position, const Game& game) const
{
	return game.scorer.get_grid_score_move(position) == 10;
}

bool MoveSolver::no_priority_ships(const Position& position, const Game& game) const
{
	return game.scorer.get_grid_score_move(position) < 2;
}

double MoveSolver::score_path(
	shared_ptr<Ship> ship, 
	const vector<Direction>& path, 
	int reach, 
	int distance_margin, 
	const Game& game
) const
{
	Position current_position = ship->position;
	int distance = game.distance_from_objective(ship);
	int cargo = ship->halite;
	int burned = 0;
	int moves = 0;

	unordered_map<Position, int> visited_positions;

	// Do not stay on spot if more important ship is passing
	if (!no_priority_ships(current_position, game) && (path[0] == Direction::STILL))
		return -9999999.0;

	for (Direction direction : path)
	{
		if (!visited_positions.count(current_position))
			visited_positions[current_position] = game.mapcell(current_position)->halite;

		int halite = visited_positions[current_position];
		int halite_to_burn = (int)floor(0.1 * halite);

		// If STILL, get halite
		if (direction == Direction::STILL)
		{
			// Extract halite from cell
			int d_halite = (int)ceil(0.25 * halite);
			visited_positions[current_position] -= d_halite;

			// Inspiration bonus
			if (game.scorer.get_grid_score_inspiration(current_position) >= 2)
				d_halite *= 3;

			cargo += d_halite;
		}
		// Try to move to next cell
		else if (cargo >= halite_to_burn)
		{
			current_position = game.game_map->directional_offset(current_position, direction);
			
			//if (
			//	game.get_constant("Test") &&
			//	game.is_two_player_game() &&
			//	enemy_at_position(current_position, game)
			//)
			//{
			//	double halite_enemy = (double)game.mapcell(current_position)->ship->halite;
			//	double score_attack_allies_nearby = max(game.scorer.get_grid_score_allies_nearby(current_position) - (1000.0 - (double)cargo), 0.0);
			//	double score_attack_enemies_nearby = max(game.scorer.get_grid_score_attack_enemies_nearby(current_position) - (1000.0 - (double)halite_enemy), 0.0);
			//	double proba_of_me_getting_back = min(score_attack_allies_nearby / (score_attack_allies_nearby + score_attack_enemies_nearby), 0.95);

			//	double score_ally = -cargo + (cargo + halite_enemy) * proba_of_me_getting_back;
			//	double score_enemy = -halite_enemy + (cargo + halite_enemy) * (1.0 - proba_of_me_getting_back);

			//	return score_ally - score_enemy;
			//}
			if (!valid_move(current_position, game) || (game.distance(ship->target_position(), current_position) > max(distance, reach) + distance_margin))
			{
				return -9999999.0;
			}
			else
			{
				cargo -= halite_to_burn;
				burned += halite_to_burn;
				moves++;
			}
		}
		// staying still totally will be captured anyway
		else
		{
			return -9999999.0;
		}
	}

	int final_distance = game.distance(current_position, ship->target_position());
	int d_distance = final_distance - distance;
	if ((distance <= 2) && (final_distance <= 2))
		d_distance = 0; // if close to objective can move freely

	double score =
		max(cargo - (double)ship->halite, 0.0) / max((double)moves, 1.0)
		//max(cargo - (double)ship->halite, 0.0) / max(sqrt((double)burned), 1.0)
		//max(cargo - (double)ship->halite, 0.0) - (double)burned
		//- (double)small_cell_malus / 10.0
		-(double)d_distance * 15.0;
		//- game.is_shipyard_or_dropoff(current_position) * 30.0;

	if (false)
	{
		string line;
		line += current_position.to_string_position();
		line += " ";
		for (Direction direction : path)
			line += to_string_direction(direction);
		line += " " + to_string(max(cargo - (double)ship->halite, 0.0));
		line += " " + to_string(burned);
		line += " " + to_string(moves);
		line += " " + to_string(d_distance * 5.0);
		line += " " + to_string(score);
		log::log(line);
	}
	
	return score;
}

bool is_null_path(const vector<Direction>& path)
{
	for (Direction direction : path)
		if (direction != Direction::STILL)
			return false;

	return true;
}

pair<Position, double> MoveSolver::find_best_extract_move(shared_ptr<Ship> ship, const Game& game, int reach, int distance_margin) const
{
	vector<double> scores((int)pow(5, reach), 0.0);

	const vector<vector<Direction>>* path_permutations = get_path_permutations(reach);
	int i = 0;
	for (const vector<Direction>& path : *path_permutations)
		scores[i++] = score_path(ship, path, reach, distance_margin, game);

	int best_score_index = distance(scores.begin(), max_element(scores.begin(), scores.end()));
	Direction best_direction = game.move_solver.get_best_direction(best_score_index, 0, reach);

	// If couldn't compute good move, try again with margin of 1 more
	if ((scores[best_score_index] == -9999999.0) || (is_null_path((*path_permutations)[best_score_index])))
	{
		log::log("No good move computed for " + ship->to_string_ship() + " trying with margin of 1.");

		i = 0;
		scores = vector<double>((int)pow(5, reach), 0.0);
		for (const vector<Direction>& path : *path_permutations)
			scores[i++] = score_path(ship, path, reach, distance_margin + 1, game);

		best_score_index = distance(scores.begin(), max_element(scores.begin(), scores.end()));
		best_direction = game.move_solver.get_best_direction(best_score_index, 0, reach);
	}

	string line = "";
	for (Direction direction : (*path_permutations)[best_score_index])
		line += to_string_direction(direction);
	log::log(ship->to_string_ship() + " best moves ahead: " + line + " with score " + to_string(scores[best_score_index]));

	return make_pair(game.game_map->directional_offset(ship->position, best_direction), scores[best_score_index]);
}

pair<Position, double> MoveSolver::find_best_action(shared_ptr<Ship> ship, const Game& game) const
{
	pair<Position, double> best_move;

	if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
	{
		// If not enough halite, extract around base target position
		if (game.distance(ship->position, ship->target_position()) <= 2)
			best_move = find_best_extract_move(ship, game, 2, 0);
		else
			best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

		log::log(ship->to_string_ship() + " creating dropoff on " + best_move.first.to_string_position());
	}

	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
	{
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
		log::log(ship->to_string_ship() + " suiciding on " + best_move.first.to_string_position());
	}

	else if (ship->is_objective(Objective_Type::BACK_TO_BASE))
	{
		// If it's worth it to stop then stop
		if (
			(game.halite_on_position(ship->position) >= 100) &&
			((int)(1.25 * (1000 - ship->halite)) >= (int)ceil(0.25 * game.halite_on_position(ship->position)))
			)
		{
			best_move = make_pair(ship->position, -game.distance_from_objective(ship));
			log::log(ship->to_string_ship() + " back to base but stopping on " + best_move.first.to_string_position());
		}
		else
		{
			best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
			log::log(ship->to_string_ship() + " back to base on " + best_move.first.to_string_position());
		}		
	}

	else if (ship->is_objective(Objective_Type::ATTACK))
	{
		best_move = make_pair(ship->target_position(), ship->objective->score);
		log::log(ship->to_string_ship() + " attacking on " + best_move.first.to_string_position());
	}

	else // Objective_Type::EXTRACT_ZONE
	{
		int reach = (game.me->ships.size() <= 30) ? 6 : 5;
		
		best_move = find_best_extract_move(ship, game, reach, 0);
	}
	
	return best_move;
}