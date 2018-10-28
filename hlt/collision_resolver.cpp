#include "collision_resolver.hpp"
#include "game.hpp"

#include <vector>
#include <unordered_map>
#include <cmath>

using namespace hlt;
using namespace std;


/*
Fill positions of enemies this turn
*/
void CollisionResolver::fill_positions_enemies(Game& game)
{
	positions_enemies.clear();
	for (vector<MapCell>& row : game.game_map->cells)
		for (MapCell& cell : row)
			if (cell.is_occupied_by_enemy(game.me->id))
				positions_enemies[cell.ship] = cell.ship->position;
}

/*
Finds the first tuples of ships that will collide. There is no ordering of importance here.
*/
unordered_map<shared_ptr<Ship>, Position> CollisionResolver::find_any_enemy_collisions(const Game& game)
{
	unordered_map<shared_ptr<Ship>, Position> collisions;

	for (auto& ship_move : game.positions_next_turn)
	{
		for (auto& enemy_ship_move : positions_enemies)
			if (
				(ship_move.second == enemy_ship_move.second) &&
				!(enemy_ship_move.second != game.my_shipyard_position())
			)
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
unordered_map<shared_ptr<Ship>, Position> CollisionResolver::find_any_collisions(const Game& game)
{
	unordered_map<shared_ptr<Ship>, Position> collisions;

	for (auto& ship_move1 : game.positions_next_turn)
	{
		for (auto& ship_move2 : game.positions_next_turn)
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
bool CollisionResolver::position_collides_with_existing(shared_ptr<Ship> ship, const Position& position, const Game& game)
{
	// Check for own collisions
	for (auto& ship_position : game.positions_next_turn)
		if ((ship_position.first != ship) && (ship_position.second == position))
			return true;

	// Check for enemy collisions
	for (auto& ship_position : positions_enemies)
		if ((ship_position.first != ship) && (ship_position.second == position))
			return true;

	return false;
}

bool CollisionResolver::is_ship_switching_places(shared_ptr<Ship> ship, Game& game) const
{
	// Check that ship's starting point is ending point of someone else
	for (auto& ship_position : game.positions_next_turn)
		if (
			(ship_position.first != ship) && 
			(ship_position.second == ship->position) && 
			(game.positions_next_turn[ship] == ship_position.second)
		)
			return true;

	return false;
}

/*
Try to change the move of the first ship in the collision tuple that moves. No importance ordering here.
*/
void CollisionResolver::edit_collisions(unordered_map<shared_ptr<Ship>, Position> collisions, Game& game)
{
	log::log("Collisions:");
	for (auto& ship_position : collisions)
		log::log(ship_position.first->to_string_ship() + " moving to " + ship_position.second.to_string_position());

	// here also sort by decreasing order of halite

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
		// If ship exchanges places with another, do not edit it
		if (is_ship_switching_places(ship, game))
		{
			continue;
		}
		// Recompute new path otherwise
		else
		{
			Position position_next_turn_old = game.positions_next_turn[ship];
			game.assign_ship_to_target_position(ship);
			Position position_next_turn_new = game.positions_next_turn[ship];

			// If no paths have been changed, stop ship
			if (position_next_turn_old == position_next_turn_new)
			{
				game.update_ship_target_position(ship, ship->position);
				log::log("No resolving, stop " + ship->to_string_ship());
			}

			return;
		}
	}
}

void CollisionResolver::exchange_ships(Game& game)
{

}

/*
Main function to resolve moves
*/
vector<Command> CollisionResolver::resolve_moves(Game& game)
{
	vector<Command> resolved_moves;

	// Initialize with all ship's first moves, eventually with enemy's positions
	fill_positions_enemies(game);

	// While own collisions exist, edit ships one by one
	unordered_map<shared_ptr<Ship>, Position> collisions = find_any_collisions(game);
	int i = 0;
	while (collisions.size())
	{
		edit_collisions(collisions, game);
		collisions = find_any_collisions(game);
		i++;

		if (i == 50)
			break;
			//log::log_vectorvector(game.grid_score);
	}

	// While enemy collisions exist, edit ships one by one
	unordered_map<shared_ptr<Ship>, Position> enemy_collisions = find_any_enemy_collisions(game);
	int j = 0;
	while (enemy_collisions.size())
	{
		edit_collisions(enemy_collisions, game);
		enemy_collisions = find_any_enemy_collisions(game);
		j++;

		if (j == 50)
			break;
	}
	
	exchange_ships(game);

	// Generate Command vector
	for (auto& ship_position : game.positions_next_turn)
	{
		Direction direction = game.game_map->get_unsafe_moves(ship_position.first->position, ship_position.second)[0];
		resolved_moves.push_back(ship_position.first->move(direction));
	}

	return resolved_moves;
}