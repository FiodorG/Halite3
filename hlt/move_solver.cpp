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

tuple<int, int> MoveSolver::score_path(shared_ptr<Ship> ship, const vector<Direction>& path, int reach, const Game& game) const
{
	// TODO: Add inspiration

	Position current_position = ship->position;
	int distance = game.distance_from_objective(ship);
	int cargo = ship->halite;
	int moves = 0;

	unordered_map<Position, int> visited_positions;

	for (Direction direction : path)
	{
		if (!visited_positions.count(current_position))
			visited_positions[current_position] = game.mapcell(current_position)->halite;

		double halite = visited_positions[current_position];

		if (halite < 100)
			halite = floor((double)(halite * halite) / 100.0);

		// If STILL, get halite
		if (direction == Direction::STILL)
		{
			int d_halite = (int)ceil(0.25 * halite);
			cargo += d_halite;
			visited_positions[current_position] -= d_halite;
		}
		// Try to move to next cell
		else if (cargo >= floor(0.1 * halite))
		{
			current_position = game.game_map->directional_offset(current_position, direction);

			if ((!valid_move(current_position, game)) || (game.distance(ship->target_position(), current_position) > max(distance, reach)))
			{
				return make_tuple(-9999999, moves);
			}
			else
			{
				cargo -= (int)floor(0.1 * halite);
				moves++;
			}
		}
		// staying still totally will be captured anyway
		else
		{
			return make_tuple(-9999999, moves);
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

	return make_tuple(cargo - ship->halite, moves);
}
	
pair<Position, int> MoveSolver::find_best_extract_move(shared_ptr<Ship> ship, const Game& game, int reach) const
{
	// Compute score on each path permutation
	int size = (int)pow(5, reach);
	vector<int> final_cargo(size, 0);
	vector<int> moves(size, 0);
	vector<double> avg_cargo(size, 0);

	const vector<vector<Direction>>* path_permutations = get_path_permutations(reach);
	int i = 0;
	for (const vector<Direction>& path : *path_permutations)
	{
		auto path_result = score_path(ship, path, reach, game);
		final_cargo[i]	 = get<0>(path_result);
		moves[i]		 = get<1>(path_result);
		avg_cargo[i]     = max((double)final_cargo[i], 0.0) / (1.0 + (double)(moves[i]));
		i++;
	}

	int best_score_index = distance(avg_cargo.begin(), max_element(avg_cargo.begin(), avg_cargo.end()));

	//log::log("Scores");
	//for (auto score : scores)
	//	log::log(to_string(score));
	
	Direction best_direction = game.move_solver.get_best_direction(best_score_index, 0, reach);

	//string line = "";
	//for (Direction direction : (*path_permutations)[best_score_index])
	//	line += to_string_direction(direction);
	//log::log(ship->to_string_ship() + " best 4 ahead moves: " + line + " with score " + to_string(final_cargo[best_score_index]) + ", direction " + to_string_direction(best_direction));

	return make_pair(game.game_map->directional_offset(ship->position, best_direction), final_cargo[best_score_index]);
}

pair<Position, int> MoveSolver::find_best_action(shared_ptr<Ship> ship, const Game& game) const
{
	pair<Position, int> best_move;

	if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
	{
		// If not enough halite, extract around base target position
		if (game.distance(ship->position, ship->target_position()) <= 2)
			best_move = find_best_extract_move(ship, game, 2);
		else
			best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
	}

	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

	else if (ship->is_objective(Objective_Type::BACK_TO_BASE))
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));

	else
	{
		int reach = (game.my_ships_number() <= 15) ? 5 : 4;

		pair<Position, int> local_move = find_best_extract_move(ship, game, reach);

		if (
			//true
			(local_move.second >= ship->missing_halite()) ||
			(game.distance(ship->position, ship->target_position()) <= reach)
			)
		{
			log::log(ship->to_string_ship() + " choosing local move to " + local_move.first.to_string_position() + " distant from " + to_string(game.distance(ship->position, ship->target_position()) <= reach));
			best_move = local_move;
		}
		else
			best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
	}
	
	return best_move;
}