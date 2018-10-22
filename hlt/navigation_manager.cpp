#include "navigation_manager.hpp"
#include "game.hpp"

#include <vector>
#include <unordered_map>
#include <cmath>

using namespace hlt;
using namespace std;

/*
Finds the first tuples of ships that will collide. There is no ordering of importance here.
*/
unordered_map<shared_ptr<Ship>, Position> NavigationManager::find_any_enemy_collisions(const Game& game)
{
	unordered_map<shared_ptr<Ship>, Position> collisions;

	for (auto& ship_move : positions_next_turn)
	{
		for (auto& enemy_ship_move : positions_enemies)
			if (ship_move.second == enemy_ship_move.second)
				collisions[enemy_ship_move.first] = enemy_ship_move.second;

		if (collisions.size())
		{
			collisions[ship_move.first] = ship_move.second;
			return collisions;
		}
	}

	return collisions;
}

/*
Finds the first tuples of ships that will collide. There is no ordering of importance here.
*/
unordered_map<shared_ptr<Ship>, Position> NavigationManager::find_any_collisions(const Game& game)
{
	unordered_map<shared_ptr<Ship>, Position> collisions;

	for (auto& ship_move1 : positions_next_turn)
	{
		for (auto& ship_move2 : positions_next_turn)
			if (
				(*ship_move1.first != *ship_move2.first) &&
				(ship_move1.second == ship_move2.second) &&
				!(
					(ship_move1.first->is_objective(Objective_Type::SUICIDE_ON_BASE) && (ship_move2.second == game.my_shipyard_position())) ||
					(ship_move2.first->is_objective(Objective_Type::SUICIDE_ON_BASE) && (ship_move1.second == game.my_shipyard_position()))
				)
			)
				collisions[ship_move2.first] = ship_move2.second;

		if (collisions.size())
		{
			collisions[ship_move1.first] = ship_move1.second;
			return collisions;
		}
	}

	return collisions;
}

/*
Checks if a new position will collide with an existing one
*/
bool NavigationManager::position_collides_with_existing(shared_ptr<Ship> ship, const Position& position)
{
	// Check for own collisions
	for (auto& ship_position : positions_next_turn)
		if ((ship_position.first != ship) && (ship_position.second == position))
			return true;

	// Check for enemy collisions
	for (auto& ship_position : positions_enemies)
		if ((ship_position.first != ship) && (ship_position.second == position))
			return true;

	return false;
}

/*
Try to change the move of the first ship in the collision tuple that moves. No importance ordering here.
*/
void NavigationManager::edit_collisions(unordered_map<shared_ptr<Ship>, Position> collisions, unordered_map<shared_ptr<Ship>, vector<Direction>> moves_queue, const Game& game)
{
	// We try to edit the first ship that comes up, no order.
	for (auto& ship_position : collisions)
	{
		shared_ptr<Ship> ship = ship_position.first;
		Position new_position = ship_position.second;

		// If not own ship, continue
		if (ship->owner != game.my_id)
		{
			continue;
		}
		// If ship not planning to move, continue
		if (ship->position == new_position)
		{
			continue;
		}
		// If ship on suicide mission, do not change it
		if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE) && (new_position == game.my_shipyard_position()))
		{
			continue;
		}
		// The first move is the colliding one, try second move.
		else if (moves_queue[ship].size() == 2)
		{
			Direction second_direction = moves_queue[ship][1];
			Position second_position = ship->position.directional_offset(second_direction);

			if (position_collides_with_existing(ship, second_position))
			{
				log::log(ship->to_string_ship() + " second move invalid, staying still");
				positions_next_turn[ship] = ship->position;
				return;
			}
			else
			{
				log::log(ship->to_string_ship() + " second move valid, using");
				positions_next_turn[ship] = second_position;
				return;
			}
		}
		// Try perpendicular moves first, else immobilize ships
		else
		{
			Direction first_direction = moves_queue[ship][0];
			Direction perpendicular_direction_left = Position::perpendicular_direction_left(first_direction);
			Direction perpendicular_direction_right = Position::perpendicular_direction_right(first_direction);
			Position perpendicular_position_left = ship->position.directional_offset(perpendicular_direction_left);
			Position perpendicular_position_right = ship->position.directional_offset(perpendicular_direction_right);

			if (!position_collides_with_existing(ship, perpendicular_position_left))
			{
				log::log(ship->to_string_ship() + " perpendicular left valid, using");
				positions_next_turn[ship] = perpendicular_position_left;
				return;
			}
			else if (!position_collides_with_existing(ship, perpendicular_position_right))
			{
				log::log(ship->to_string_ship() + " perpendicular right valid, using");
				positions_next_turn[ship] = perpendicular_position_right;
				return;
			}
			// Else immobilize that ship
			else
			{
				log::log(ship->to_string_ship() + " staying still");
				positions_next_turn[ship] = ship->position;
				return;
			}
		}
	}
}

/*
Fills positions of own ships on next turn
*/
void NavigationManager::fill_positions_next_turn(const unordered_map<shared_ptr<Ship>, vector<Direction>>& moves_queue, const Game& game)
{
	positions_next_turn.clear();
	for (auto& ship_move : moves_queue)
	{
		shared_ptr<Ship> ship = ship_move.first;

		if ((ship_move.second.size() > 0) && (ship->halite >= ceil(0.1 * game.game_map->at(ship)->halite)))
			// either no move or can't move for lack of resource
			positions_next_turn[ship] = ship->position.directional_offset(ship_move.second[0]);
		else
			// or move
			positions_next_turn[ship] = ship->position;
	}
}

/*
Fill positions of enemies this turn
*/
void NavigationManager::fill_positions_enemies(const Game& game)
{
	positions_enemies.clear();
	for (vector<MapCell>& row : game.game_map->cells)
		for (MapCell& cell : row)
			if (cell.is_occupied_by_enemy(game.me->id))
				positions_enemies[cell.ship] = cell.ship->position;
}

/*
Main function to resolve moves
*/
vector<Command> NavigationManager::resolve_moves(const unordered_map<shared_ptr<Ship>, vector<Direction>>& moves_queue, const Game& game)
{
	vector<Command> resolved_moves;

	// Initialize with all ship's first moves, eventually with enemy's positions
	fill_positions_next_turn(moves_queue, game);
	fill_positions_enemies(game);

	// While own collisions exist, edit ships one by one
	unordered_map<shared_ptr<Ship>, Position> collisions = find_any_collisions(game);
	while (collisions.size())
	{
		edit_collisions(collisions, moves_queue, game);
		collisions = find_any_collisions(game);
	}

	// While enemy collisions exist, edit ships one by one
	unordered_map<shared_ptr<Ship>, Position> enemy_collisions = find_any_enemy_collisions(game);
	while (enemy_collisions.size())
	{
		edit_collisions(enemy_collisions, moves_queue, game);
		enemy_collisions = find_any_enemy_collisions(game);
	}
	
	// Generate Command vector
	for (auto& ship_position : positions_next_turn)
	{
		Direction direction = game.game_map->get_unsafe_moves(ship_position.first->position, ship_position.second)[0];
		resolved_moves.push_back(ship_position.first->move(direction));
	}

	return resolved_moves;
}