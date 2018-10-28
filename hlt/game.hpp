#pragma once

#include "game_map.hpp"
#include "player.hpp"
#include "types.hpp"
#include "collision_resolver.hpp"
#include "pathfinder.hpp"
#include "log.hpp"

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

		// For timer
		clock_t start;

		// Movement management
		unique_ptr<CollisionResolver> collision_resolver;
		PathFinder pathfinder;

		unordered_map<shared_ptr<Ship>, Position> positions_next_turn;
		vector<Command> command_queue;

		// Scoring
		vector<vector<int>> grid_score;
		int total_halite;

		Game();

		void resolve_moves()
		{
			log::log("Resolving moves");

			command_queue = collision_resolver->resolve_moves(*this);
		}

		bool existing_objective_to_cell(const MapCell& cell) const
		{
			for (const auto& ship_iterator : me->ships)
			{
				shared_ptr<Ship> ship = ship_iterator.second;

				if (ship->has_objective() && (ship->target_position() == cell.position))
					return true;
			}
			
			return false;
		}

		bool better_neighboring_cell_exists(const Position& position)
		{
			int halite = mapcell(position)->halite;
			int halite_over_one_turn = (int)ceil(0.25 * halite);
			int halite_over_two_turn = halite_over_one_turn + (int)ceil(0.25 * (halite - halite_over_one_turn));
			int north_halite = mapcell(game_map->directional_offset(position, Direction::NORTH))->halite;
			int south_halite = mapcell(game_map->directional_offset(position, Direction::SOUTH))->halite;
			int east_halite  = mapcell(game_map->directional_offset(position, Direction::EAST))->halite;
			int west_halite  = mapcell(game_map->directional_offset(position, Direction::WEST))->halite;

			if (
				((-floor(0.1 * halite) + ceil(0.25 * north_halite)) > halite_over_two_turn) ||
				((-floor(0.1 * halite) + ceil(0.25 * south_halite)) > halite_over_two_turn) ||
				((-floor(0.1 * halite) + ceil(0.25 * east_halite)) > halite_over_two_turn) ||
				((-floor(0.1 * halite) + ceil(0.25 * west_halite)) > halite_over_two_turn)
			)
				return true;
			else
				return false;
		}

		void assign_ship_to_target_position(shared_ptr<Ship> ship)
		{
			ship->set_assigned();

			if (game_map->ship_can_move(ship))
			{
				// Enough halite to move
				log::log("Assigning: " + ship->to_string_ship());
				
				Position target_position = pathfinder.compute_shortest_path(ship->position, ship->target_position(), *this);
				update_ship_target_position(ship, target_position);
			}
			else
			{
				// Not enough halite to move
				log::log("Staying still: " + ship->to_string_ship());

				update_ship_target_position(ship, ship->position);
			}
		}
		void update_ship_target_position(shared_ptr<Ship> ship, const Position& position)
		{
			if (positions_next_turn.count(ship))
				flush_grid_score(positions_next_turn[ship]);

			positions_next_turn[ship] = position;
			add_self_ships_to_grid_score(ship, position);
		}

		void update_grid_score();
		void add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position);
		void flush_grid_score(const Position& position);
		int get_grid_score(const Position& position) const { return grid_score[position.y][position.x]; }

		void generate_new_ships()
		{
			if (
				turns_remaining_percent() >= 0.33 &&
				me->halite >= constants::SHIP_COST &&
				!position_occupied_next_turn(my_shipyard_position()) &&
				my_ships_number() < max_allowed_ships()
				)
				command_queue.push_back(me->shipyard->spawn());
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
				allowed_ships = 24;
				break;
			case 40:
				allowed_ships = 28 - players.size();
				break;
			case 48:
				allowed_ships = 32 - players.size();
				break;
			case 56:
				allowed_ships = 36 - players.size();
				break;
			case 64:
				allowed_ships = 40 - players.size();
				break;
			default:
				log::log("Unknown map width");
				exit(1);
			}
			return allowed_ships;
		}

		void fudge_ship_if_base_blocked();


		/* Utilities */
		MapCell* mapcell(const Position& position) { return game_map->at(position); }
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

		double turn_percent() const { return (double)turn_number / (double)constants::MAX_TURNS; }
		int turns_remaining() const { return constants::MAX_TURNS - turn_number; }
		double turns_remaining_percent() const { return (double)(constants::MAX_TURNS - turn_number) / (double)constants::MAX_TURNS; }
		int my_ships_number() const { return me->ships.size(); }

		Position my_shipyard_position() const { return me->shipyard->position; }
		shared_ptr<Ship> ship_on_shipyard() const { return game_map->at(my_shipyard_position())->ship; }

		bool position_occupied_next_turn(const Position& position)
		{
			for (auto& ship_position : positions_next_turn)
				if (ship_position.second == position)
					return true;

			return false;
		}


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
