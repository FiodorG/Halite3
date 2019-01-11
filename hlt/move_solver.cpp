#include "move_solver.hpp"
#include "game.hpp"

#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace hlt;
using namespace std;

double MoveSolver::score_path(shared_ptr<Ship> ship, const vector<Direction>& path, const Game& game) const
{
	Position initial_position = ship->position;
	Position current_position = ship->position;
	int distance = game.distance_from_objective(ship);
	int cargo = ship->halite;
	int moves = 0;
	double hard_no = -99999999.0;
	double soft_no = -9999999.0;
	double distance_multiplier = game.is_two_player_game() ? 25.0 : 40.0;
	bool four_player_game = game.is_four_player_game();
	bool can_attack_4p = (ship->halite < 500) && four_player_game && (game.turns_remaining_percent() < 0.33);
	//bool can_attack_2p = (ship->halite < 500) && game.is_two_player_game();
	bool can_attack = can_attack_4p; // || can_attack_2p;
	int turn = 0;
	double score = 0.0;

	unordered_map<Position, int> visited_positions;

	// Do not stay on spot if more important ship is passing
	if ((path[0] == Direction::STILL) && (game.scorer.get_grid_score_move(current_position) == 2)) 
		return hard_no;

	// If can't stay still return hard no as the enemy next to ship will try to kill for sure, the other ones not sure
	if ((path[0] == Direction::STILL) && (game.scorer.get_grid_score_can_stay_still(current_position) <= 0.0))
		return hard_no;
	
	for (Direction direction : path)
	{
		if (!visited_positions.count(current_position))
			visited_positions[current_position] = game.mapcell(current_position)->halite;

		int halite = visited_positions[current_position];
		int halite_to_burn = (int)floor(0.1 * halite);

		// If STILL, get halite
		if (direction == Direction::STILL)
		{
			int d_halite = (int)ceil(0.25 * halite);
			visited_positions[current_position] -= d_halite;

			int inspiration = game.scorer.get_grid_score_inspiration(current_position);
			if (inspiration >= 2)
				d_halite *= 3;

			cargo += d_halite;
			score += ((double)d_halite + game.scorer.get_grid_score_neighbor_cell(current_position)) * pow(0.9, moves); // *(1.0 + 0.1 * max(inspiration - 2, 0));
		}
		// Try to move to next cell
		else if (cargo >= halite_to_burn)
		{
			current_position = game.game_map->directional_offset(current_position, direction);
			int score_move = game.scorer.get_grid_score_move(current_position);
			int current_distance = game.distance(initial_position, current_position);

			//if (four_player_game && (score_move == 10) && (current_distance <= 2) && (score_dangerous_cell > 800.0))
			//{
			//	cargo -= halite_to_burn;
			//	score -= halite_to_burn * pow(0.9, moves);
			//	moves++;
			//}
			//else 
			if (can_attack && (score_move == 10) && (current_distance <= 2) && (game.scorer.get_score_ship_can_move_to_dangerous_cell(ship, current_position) > 400.0))
			{
				return game.scorer.get_score_ship_can_move_to_dangerous_cell(ship, current_position) * pow(0.9, turn) / (1.0 + (double)moves);
			}
			// if positive combat expectation of moving to cell, then do so
			else if ((score_move == 9) && (current_distance <= 3) && (game.scorer.get_score_ship_can_move_to_dangerous_cell(ship, current_position) > 200.0))
			{
				cargo -= halite_to_burn;
				score -= halite_to_burn * pow(0.9, moves);
				moves++;
			}
			// if move next to enemy, return score of doing so
			else if ((score_move == 9) && (current_distance == 1))
			{
				return soft_no - game.scorer.get_score_ship_move_to_position(ship, current_position, game);
				//return soft_no + game.scorer.get_score_ship_can_move_to_dangerous_cell(ship, current_position);
			}
			// If ally or enemy in other cell, hard no
			else if (score_move > 0)
			{
				return hard_no;
			}
			else
			{
				cargo -= halite_to_burn;
				score -= halite_to_burn * pow(0.9, moves);
				moves++;
			}
		}
		// staying still will be captured anyway
		else
		{
			return hard_no;
		}

		turn++;
	}

	int final_distance = game.distance(current_position, ship->target_position());
	int d_distance = final_distance - distance;

	// if close to objective can move freely
	if ((distance <= 2) && (final_distance <= 2))
		d_distance = 0;

	score = score - (double)d_distance * distance_multiplier;

	if (false)
	{
		string line;
		line += current_position.to_string_position();
		line += " ";
		for (Direction direction : path)
			line += to_string_direction(direction);
		line += " " + to_string(cargo - (double)ship->halite);
		line += " " + to_string(moves);
		line += " " + to_string(d_distance * distance_multiplier);
		line += " " + to_string(score);
		log::log(line);
	}
	
	return score;
}

pair<Position, double> MoveSolver::find_best_extract_move(shared_ptr<Ship> ship, const Game& game, int reach) const
{
	vector<double> scores((int)pow(5, reach), 0.0);

	const vector<vector<Direction>>* path_permutations = get_path_permutations(reach);
	int i = 0;
	for (const vector<Direction>& path : *path_permutations)
		scores[i++] = score_path(ship, path, game);

	int best_score_index = distance(scores.begin(), max_element(scores.begin(), scores.end()));
	Direction best_direction = game.move_solver.get_best_direction(best_score_index, 0, reach);

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
		best_move = make_pair(ship->target_position(), -game.distance_from_objective(ship));
		log::log(ship->to_string_ship() + " creating dropoff on " + best_move.first.to_string_position());
	}

	else if (ship->is_objective(Objective_Type::BLOCK_ENEMY_BASE))
	{
		best_move = make_pair(ship->target_position(), ship->objective->score);
		log::log(ship->to_string_ship() + " blocking base on " + best_move.first.to_string_position());
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
			((int)(1.25 * (1000 - ship->halite)) >= (int)ceil(0.25 * game.halite_on_position(ship->position))) &&
			!game.enemy_in_adjacent_cell(ship->position)
			)
		{
			best_move = make_pair(ship->position, -game.distance_from_objective(ship));
			ship->objective->target_position = ship->position;
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
		int reach = (game.me->ships.size() <= 50) ? 6 : 5;
		
		best_move = find_best_extract_move(ship, game, reach);
	}
	
	return best_move;
}