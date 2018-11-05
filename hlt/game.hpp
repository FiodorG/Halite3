#pragma once

#include "game_map.hpp"
#include "player.hpp"
#include "types.hpp"
#include "collision_resolver.hpp"
#include "pathfinder.hpp"
#include "log.hpp"
#include "scorer.hpp"
#include "move_solver.hpp"

#include <vector>
#include <iostream>
#include <cmath>

using namespace std;

namespace hlt 
{
    struct Game 
	{
        int turn_number;
        PlayerId my_id;
        vector<shared_ptr<Player>> players;
        shared_ptr<Player> me;
        unique_ptr<GameMap> game_map;
		unordered_map<string, int> constants;
		int dropoffs;

		// For timer
		clock_t start;

		// Movement management
		CollisionResolver collision_resolver;
		PathFinder pathfinder;
		
		unordered_map<shared_ptr<Ship>, Position> positions_next_turn;
		vector<Command> command_queue;

		// Scoring
		Scorer scorer;

		// Move Solver
		MoveSolver move_solver;

		Game(unordered_map<string, int> constants);

		void resolve_moves()
		{
			log::log("Resolving moves");

			command_queue = collision_resolver.resolve_moves(*this);
		}

		void assign_ship_to_target_position(shared_ptr<Ship> ship, const Position& position)
		{
			ship->set_assigned();

			if (ship_can_move(ship) || ship->is_objective(Objective_Type::MAKE_DROPOFF))
			{
				// Enough halite to move
				//log::log("Assigning: " + ship->to_string_ship());
				
				Position target_position = pathfinder.compute_shortest_path(ship->position, position, *this);
				update_ship_target_position(ship, target_position);
			}
			else
			{
				// Not enough halite to move
				//log::log("Staying still: " + ship->to_string_ship());

				update_ship_target_position(ship, ship->position);
			}
		}

		void update_ship_target_position(shared_ptr<Ship> ship, const Position& position)
		{
			if (positions_next_turn.count(ship))
				scorer.flush_grid_score(positions_next_turn[ship]);

			positions_next_turn[ship] = position;
			scorer.add_self_ships_to_grid_score(ship, position);
		}

		void generate_new_ships()
		{
			if (
				turns_remaining_percent() >= 0.33 &&
				me->halite >= constants::SHIP_COST &&
				!position_occupied_next_turn(my_shipyard_position()) &&
				my_ships_number() <= max_allowed_ships()
				)
				command_queue.push_back(me->shipyard->spawn());
		}

		bool should_spawn_dropoff(const shared_ptr<Ship> ship)
		{
			if (
				(me->halite + ship->halite + mapcell(ship)->halite >= 5 + constants::DROPOFF_COST) &&
				(dropoffs < max_allowed_dropoffs()) &&
				(game_map->calculate_distance(ship->position, get_closest_shipyard_or_dropoff(ship)) > (int)ceil(0.5 * game_map->width)) &&
				// keep for last as it's the longest to compute
				(game_map->halite_around_position(ship->position, get_constant("Dropoff: distance nearby")) > get_constant("Dropoff: halite nearby"))
				)
			{
				log::log("Spawning dropoff at " + ship->to_string_ship());
				me->halite -= constants::DROPOFF_COST - ship->halite - mapcell(ship)->halite;
				dropoffs++;
				return true;
			}

			return false;
		}

		int max_allowed_dropoffs() const
		{
			int allowed_dropoffs = 0;
			switch (game_map->width)
			{
			case 32:
				allowed_dropoffs = (players.size() == 2) ? 1 : 0;
				break;
			case 40:
				allowed_dropoffs = 1;
				break;
			case 48:
				allowed_dropoffs = 2;
				break;
			case 56:
				allowed_dropoffs = (players.size() == 2) ? 3 : 2;
				break;
			case 64:
				allowed_dropoffs = (players.size() == 2) ? 3 : 2;
				break;
			default:
				log::log("Unknown map width");
				exit(1);
			}
			return allowed_dropoffs;
		}

		int max_allowed_ships() const
		{
			// assumes ships make 5 deliveries on average
			//int number_players = min((int)players.size(), 3);
			//return (int)ceil((double)total_halite / (double)number_players / 900.0 / 5.0);

			int allowed_ships = 0;
			switch (game_map->width)
			{
			case 32:
				allowed_ships = (players.size() == 2) ? get_constant("Max Ships 2p: 32") : get_constant("Max Ships 4p: 32");
				break;
			case 40:
				allowed_ships = (players.size() == 2) ? get_constant("Max Ships 2p: 40") : get_constant("Max Ships 4p: 40");
				break;
			case 48:
				allowed_ships = (players.size() == 2) ? get_constant("Max Ships 2p: 48") : get_constant("Max Ships 4p: 48");
				break;
			case 56:
				allowed_ships = (players.size() == 2) ? get_constant("Max Ships 2p: 56") : get_constant("Max Ships 4p: 56");
				break;
			case 64:
				allowed_ships = (players.size() == 2) ? get_constant("Max Ships 2p: 64") : get_constant("Max Ships 4p: 64");
				break;
			default:
				log::log("Unknown map width");
				exit(1);
			}
			return allowed_ships;
		}

		void fudge_ship_if_base_blocked();


		/* Utilities */
		MapCell* mapcell(const Position& position) const { return game_map->at(position); }
		MapCell* mapcell(shared_ptr<Ship> ship) const { return game_map->at(ship->position); }
		MapCell* mapcell(int x, int y) const { return game_map->at(y, x); }
		bool enemy_in_cell(const MapCell& cell) const { return cell.is_occupied_by_enemy(my_id); }
		bool enemy_in_cell(const Position& position) const { return game_map->at(position)->is_occupied_by_enemy(my_id); }
		bool enemy_in_adjacent_cell(const Position& position) const
		{
			return
				game_map->at(game_map->directional_offset(position, Direction::NORTH))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::SOUTH))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::EAST))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::WEST))->is_occupied_by_enemy(my_id) ||
				game_map->at(position)->is_occupied_by_enemy(my_id);
		}
		bool ally_in_cell(const Position& position) const { return game_map->at(position)->is_occupied_by_ally(my_id); }

		int get_constant(string name) const { return constants.at(name); }
		double turn_percent() const { return (double)turn_number / (double)constants::MAX_TURNS; }
		double turns_remaining_percent() const { return (double)(constants::MAX_TURNS - turn_number) / (double)constants::MAX_TURNS; }
		int turns_remaining() const { return constants::MAX_TURNS - turn_number; }
		int my_ships_number() const { return me->ships.size(); }
		int my_dropoff_number() const { return me->dropoffs.size(); }

		Position my_shipyard_position() const { return me->shipyard->position; }
		shared_ptr<Ship> ship_on_shipyard() const { return game_map->at(my_shipyard_position())->ship; }
		Position get_closest_shipyard_or_dropoff(shared_ptr<Ship> ship) const
		{
			return get_closest_shipyard_or_dropoff(ship->position);
		}
		Position get_closest_shipyard_or_dropoff(const Position& position) const
		{
			int min_distance = game_map->calculate_distance(position, my_shipyard_position());
			Position closest_shipyard = my_shipyard_position();

			for (auto& dropoff_iterator : me->dropoffs)
			{
				int distance = game_map->calculate_distance(position, dropoff_iterator.second->position);

				if (distance <= min_distance)
				{
					min_distance = distance;
					closest_shipyard = dropoff_iterator.second->position;
				}
			}

			return closest_shipyard;
		}
		bool is_shipyard_or_dropoff(const Position& position) const { return mapcell(position)->is_shipyard_or_dropoff(my_id); }
		bool position_occupied_next_turn(const Position& position)
		{
			for (auto& ship_position : positions_next_turn)
				if (ship_position.second == position)
					return true;

			return false;
		}

		int distance(const Position& position1, const Position& position2) const {  return game_map->calculate_distance(position1, position2); }
		int distance_from_objective(shared_ptr<Ship> ship) const { return game_map->calculate_distance(ship->position, ship->target_position()); }
		bool ship_can_move(shared_ptr<Ship> ship) const { return ship->halite >= (int)floor(0.1 * mapcell(ship)->halite); }

		/* Logging */
		void log_start_turn()
		{
			log::log("START TURN");

			// Log all player's ships
			//log::log("Ships:");
			//for (auto& ship_iterator : me->ships)
			//	log::log(ship_iterator.second->to_string_ship());

			// Log all enemys ships
			//log::log("Enemy ships:");
			//for (vector<MapCell>& row : game_map->cells)
			//	for (MapCell& cell : row)
			//		if (cell.is_occupied_by_enemy(me->id))
			//			log::log(cell.ship->to_string_ship());

			//log::log("");
		}
		void log_end_turn()
		{
			log::log("END TURN");

			// Log all player's ships
			//log::log("Ships(" + to_string(my_ships_number()) + "):");
			//for(auto& ship_iterator : me->ships)
			//	log::log(ship_iterator.second->to_string_ship());
		
			// Log all predicted direction
			for (auto& ship_position : positions_next_turn)
				log::log(ship_position.first->to_string_ship() + " moving to " + ship_position.second.to_string_position());

			// Log all commands
			//for (auto& command: command_queue)
			//	log::log(command);

			log::log("Time taken: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));
			log::log("");
		}


		/* functions for new turn */
        void ready(const string& name, unsigned int rng_seed);
        void update_frame();
        bool end_turn(const vector<Command>& commands);
    };
}
