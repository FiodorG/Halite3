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
	vector<Direction> directions = { Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST, Direction::STILL };

	vector<vector<Direction>> all_path_permutations((int)pow(5, move_number));

	if (move_number == 3)
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
	else
	{
		log::log("Error: move_number invalid");
		exit(1);
	}

	return all_path_permutations;
}

bool MoveSolver::valid_move(const Position& position, const Game& game) const
{
	return game.scorer.get_grid_score_move(position) == 0;
}

int MoveSolver::final_cargo_on_path(shared_ptr<Ship> ship, const vector<Direction>& path, const Game& game) const
{
	// TODO: Add inspiration

	Position current_position = ship->position;
	int cargo = ship->halite;

	unordered_map<Position, int> visited_positions;

	for (Direction direction : path)
	{
		if (!visited_positions.count(current_position))
			visited_positions[current_position] = game.mapcell(current_position)->halite;

		int halite = visited_positions[current_position];

		// If STILL, get halite
		if (direction == Direction::STILL)
		{
			int d_halite = (int)ceil(0.25 * halite);
			cargo += d_halite;
			visited_positions[current_position] -= d_halite;
		}
		// Try to move to next cell
		else if (cargo >= (int)floor(0.1 * halite))
		{
			current_position = game.game_map->directional_offset(current_position, direction);

			if (!valid_move(current_position, game))
			{
				return -999999999;
			}
			else
			{
				cargo -= (int)floor(0.1 * halite);
			}
		}
		else
		{
			return -999999999;
		}
	}

	//string line = "";
	//for (Direction dir : path)
	//	line += to_string_direction(dir);
	//log::log(line);
	//log::log("cargo:" + to_string(cargo));
	//for (auto& it : visited_positions)
	//	log::log(it.first.to_string_position() + ": " + to_string(it.second));
	//log::log("");

	return cargo;
}
	
pair<Position, int> MoveSolver::find_best_extract_move(shared_ptr<Ship> ship, const Game& game) const
{
	// Compute score on each path permutation
	vector<int> scores(game.move_solver.all_path_permutations.size(), 0);
	int i = 0;
	for (const vector<Direction>& path : all_path_permutations)
		scores[i++] = final_cargo_on_path(ship, path, game);

	// Find best move among the computed ones
	int best_score_index = distance(scores.begin(), max_element(scores.begin(), scores.end()));

	//string line = "";
	//for (Direction direction : all_path_permutations[best_score_index])
	//	line += to_string_direction(direction);
	//log::log(ship->to_string_ship() + " best 4 ahead moves: " + line + " with score " + to_string(scores[best_score_index]));

	return make_pair(game.game_map->directional_offset(ship->position, all_path_permutations[best_score_index][0]), scores[best_score_index]);
}

pair<Position, int> MoveSolver::find_best_action(shared_ptr<Ship> ship, const Game& game) const
{
	pair<Position, int> best_move;

	if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

	else if (ship->is_objective(Objective_Type::BACK_TO_BASE))
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

	else if (ship->is_objective(Objective_Type::EXTRACT_ZONE))
	{
		pair<Position, int> local_move = find_best_extract_move(ship, game);

		if (
			//(local_move.second >= 20) && 
			((local_move.second >= 0.8 * ship->objective_score()) ||
			(game.distance(ship->position, ship->target_position()) <= 4))
		)
			best_move = local_move;
		else
			best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
	}
	
	return best_move;
}

vector<shared_ptr<Ship>> MoveSolver::filter_ships_without_actions(unordered_map<shared_ptr<Ship>, int> ships_without_actions) const
{
	bool has_dropoff = false;
	bool has_suicide = false;
	bool has_back_to_base = false;

	for (const auto& ship : ships_without_actions)
	{
		if (ship.first->is_objective(Objective_Type::MAKE_DROPOFF))
		{
			has_dropoff = true;
			break;
		}
		else if (ship.first->is_objective(Objective_Type::SUICIDE_ON_BASE))
		{
			has_suicide = true;
			break;
		}
		else if (ship.first->is_objective(Objective_Type::BACK_TO_BASE))
		{
			has_back_to_base = true;
			break;
		}
	}

	vector<shared_ptr<Ship>> filtered_ships;
	if (has_dropoff)
	{
		for (const auto& ship : ships_without_actions)
			if (ship.first->is_objective(Objective_Type::MAKE_DROPOFF))
				filtered_ships.push_back(ship.first);
	}
	else if (has_suicide)
	{
		for (const auto& ship : ships_without_actions)
			if (ship.first->is_objective(Objective_Type::SUICIDE_ON_BASE))
				filtered_ships.push_back(ship.first);
	}
	else if (has_back_to_base)
	{
		for (const auto& ship : ships_without_actions)
			if (ship.first->is_objective(Objective_Type::BACK_TO_BASE))
				filtered_ships.push_back(ship.first);
	}
	else
	{
		for (const auto& ship : ships_without_actions)
			if (ship.first->is_objective(Objective_Type::EXTRACT_ZONE))
				filtered_ships.push_back(ship.first);
	}

	return filtered_ships;
}