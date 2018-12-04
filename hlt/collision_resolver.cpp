#include "collision_resolver.hpp"
#include "game.hpp"
#include "priority_queue.hpp"

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
		if (ship_move.first->is_objective(Objective_Type::ATTACK) || ship_move.first->is_objective(Objective_Type::BLOCK_ENEMY_BASE))
			continue;

		for (auto& enemy_ship_move : positions_enemies)
			if (
				(ship_move.second == enemy_ship_move.second) &&
				!game.is_shipyard_or_dropoff(enemy_ship_move.second)
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
					(ship_move1.first->is_objective(Objective_Type::SUICIDE_ON_BASE) && game.is_shipyard_or_dropoff(ship_move2.second)) ||
					(ship_move2.first->is_objective(Objective_Type::SUICIDE_ON_BASE) && game.is_shipyard_or_dropoff(ship_move1.second))
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
	// First order ships/positions in increasing order of halite
	PriorityQueue<shared_ptr<Ship>, int> ships_in_priority;
	for (auto& ship_iterator : collisions)
		ships_in_priority.put(ship_iterator.first, ship_iterator.first->halite);

	list<shared_ptr<Ship>> collisions_ordered;
	while (!ships_in_priority.empty())
		collisions_ordered.push_back(ships_in_priority.get());

	log::log("Collisions:");
	for (auto& ship : collisions_ordered)
		log::log(ship->to_string_ship() + " moving to " + collisions[ship].to_string_position());

	// We try to edit the first ship that comes up, no order.
	for (auto& ship : collisions_ordered)
	{
		Position new_position = collisions[ship];

		// If not own ship, continue
		if (ship->owner != game.my_id)
		{
			continue;
		}
		// If ship not planning to move, continue
		//if (ship->position == new_position)
		if (!game.ship_can_move(ship))
		{
			continue;
		}
		// If ship on suicide mission, do not change it
		if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE) && game.is_shipyard_or_dropoff(new_position))
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
			log::log("Trying to fudge " + ship->to_string_ship());
			Position position_next_turn_old = game.positions_next_turn[ship];
			Position new_position = game.pathfinder.compute_shortest_path(ship->position, ship->target_position(), game);
			if (
				position_collides_with_existing(ship, new_position, game) ||
				(position_next_turn_old == new_position)
			)
			{
				game.update_ship_target_position(ship, ship->position);
				log::log("No resolving, stop " + ship->to_string_ship());
			}
			else
				game.update_ship_target_position(ship, new_position);

			return;
		}
	}
}

/*
Third pass to exchange two immobilized ships if doing so will both make them get closer to their target
*/
void CollisionResolver::exchange_ships(Game& game)
{
	unordered_map<EntityId, shared_ptr<Ship>> ships_selected;
	list<tuple<shared_ptr<Ship>, shared_ptr<Ship>>> ships_to_switch;

	// Select ships which can be exchanged so that they get closer to their targets
	for (auto& ship_position1 : game.positions_next_turn)
	{
		shared_ptr<Ship> ship1 = ship_position1.first;

		for (auto& ship_position2 : game.positions_next_turn)
		{
			shared_ptr<Ship> ship2 = ship_position2.first;

			if (
				(game.game_map->calculate_distance(ship1->position, ship2->position) == 1) && // ships are contiguous
				game.ship_can_move(ship1) && // ship1 can move
				game.ship_can_move(ship2) && // ship2 can move
				!ships_selected.count(ship1->id) && // ship1 not already selected
				!ships_selected.count(ship2->id) && // ship2 not already selected
				!ship1->is_at_objective() &&  // ship1 not at objective 
				!ship2->is_at_objective() &&  // ship2 not at objective
				(ship_position1.second == ship1->position) && // ship1 immobilized
				(ship_position2.second == ship2->position)    // ship2 immobilized
				)
			{
				int ship1_distance_to_objective = game.game_map->calculate_distance(ship1->target_position(), ship1->position);
				int ship2_distance_to_objective = game.game_map->calculate_distance(ship2->target_position(), ship2->position);

				if (
					(game.game_map->calculate_distance(ship1->target_position(), ship2->position) < ship1_distance_to_objective) && // ship1 target is closer from ship2 position
					(game.game_map->calculate_distance(ship2->target_position(), ship1->position) < ship2_distance_to_objective)    // ship2 target is closer from ship1 position
					)
				{
					ships_selected[ship1->id] = ship1;
					ships_selected[ship2->id] = ship2;

					ships_to_switch.push_back(make_tuple(ship1, ship2));

					goto endinnerloop;
				}
			}	
		}

	endinnerloop:;
	}

	// Invert their positions
	for (auto& ships_tuple : ships_to_switch)
	{
		log::log("Exchanging " + get<0>(ships_tuple)->to_string_ship() + " and " + get<1>(ships_tuple)->to_string_ship());
		game.positions_next_turn[get<0>(ships_tuple)] = get<1>(ships_tuple)->position;
		game.positions_next_turn[get<1>(ships_tuple)] = get<0>(ships_tuple)->position;
	}
}

void CollisionResolver::exchange_ships_on_base(Game& game)
{
	// In the last turns during suicide not good idea to do this
	if (game.turns_remaining() <= 25)
		return;

	list<tuple<shared_ptr<Ship>, shared_ptr<Ship>>> ships_to_switch;

	for (auto& ship_position1 : game.positions_next_turn)
	{
		shared_ptr<Ship> ship1 = ship_position1.first;
		Position position1 = ship_position1.second;


		// Select only ships on shipyard which aren't going to move
		if ((position1 != ship1->position) || !game.is_shipyard_or_dropoff(ship1->position))
			continue;

		// Select best returning ship to switch with
		shared_ptr<Ship> ship_to_switch_with = ship1;
		int halite_of_ship_to_switch_with = INT_MIN;

		for (auto& ship_position2 : game.positions_next_turn)
		{
			shared_ptr<Ship> ship2 = ship_position2.first;

			if (
				(ship2 != ship1) &&
				(ship2->is_objective(Objective_Type::BACK_TO_BASE)) && // ship2 is going back to base
				(game.distance(ship1->position, ship2->position) == 1) && // ships are contiguous
				(ship2->halite > halite_of_ship_to_switch_with)
				)
			{
				ship_to_switch_with = ship2;
				halite_of_ship_to_switch_with = ship2->halite;
			}
		}

		if (ship1 != ship_to_switch_with)
			ships_to_switch.push_back(make_tuple(ship1, ship_to_switch_with));
	}

	// Invert their positions
	for (auto& ships_tuple : ships_to_switch)
	{
		log::log("Exchanging at dropoff " + get<0>(ships_tuple)->to_string_ship() + " and " + get<1>(ships_tuple)->to_string_ship());
		game.positions_next_turn[get<0>(ships_tuple)] = get<1>(ships_tuple)->position;
		game.positions_next_turn[get<1>(ships_tuple)] = get<0>(ships_tuple)->position;
	}
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
	for (int repeat = 0; repeat < 2; repeat++)
	{
		unordered_map<shared_ptr<Ship>, Position> collisions = find_any_collisions(game);
		int i = 0;
		while (collisions.size())
		{
			edit_collisions(collisions, game);
			collisions = find_any_collisions(game);
			i++;

			if (i == 50)
				break;
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
	}
	
	exchange_ships(game);
	exchange_ships_on_base(game);

	// Generate Command vector
	for (auto& ship_position : game.positions_next_turn)
	{
		if (game.objective_manager.can_spawn_dropoff(ship_position.first, game))
		{
			resolved_moves.push_back(ship_position.first->make_dropoff());
		}
		else
		{
			Direction direction = game.game_map->get_move(ship_position.first->position, ship_position.second);
			resolved_moves.push_back(ship_position.first->move(direction));
		}
	}

	return resolved_moves;
}