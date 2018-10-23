#pragma once

#include "game_map.hpp"
#include "player.hpp"
#include "types.hpp"
#include "navigation_manager.hpp"
#include "game_grid.hpp"
#include "log.hpp"

#include <vector>
#include <iostream>

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
		unique_ptr<NavigationManager> navigation_manager;
		unordered_map<shared_ptr<Ship>, vector<Direction>> moves_queue;
		vector<Command> command_queue;
		GameGrid game_grid;

		Game();

		void resolve_moves()
		{
			log::log("Resolving moves");
			command_queue = navigation_manager->resolve_moves(moves_queue, *this);
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

		bool enemy_in_cell(const MapCell& cell) const
		{
			return cell.is_occupied_by_enemy(my_id);
		}

		bool enemy_in_cell(const Position& position) const
		{
			return game_map->at(position)->is_occupied_by_enemy(my_id);
		}

		int turns_remaining() const
		{
			return constants::MAX_TURNS - turn_number;
		}

		Position my_shipyard_position() const
		{
			return me->shipyard->position;
		}

		int my_ships_number() const
		{
			return me->ships.size();
		}

		int max_allowed_ships() const
		{
			int allowed_ships = 0;
			switch (game_map->width)
			{
				case 32:
					allowed_ships = 24 - players.size();
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

		Position compute_shortest_path(const Position& source_position, const Position& target_position) const
		{
			return game_grid.compute_shortest_path(source_position, target_position, *this);
		}


		/* Logging */
		void log_start_turn()
		{
			log::log("START TURN");

			// Log all player's ships
			log::log("Ships:");
			for (auto& ship_iterator : me->ships)
				log::log(ship_iterator.second->to_string_ship());

			// Log all enemys ships
			//log::log("Enemy ships:");
			//for (vector<MapCell>& row : game_map->cells)
			//	for (MapCell& cell : row)
			//		if (cell.is_occupied_by_enemy(me->id))
			//			log::log(cell.ship->to_string_ship());

			log::log("");
		}

		void log_end_turn()
		{
			log::log("END TURN");

			// Log all player's ships
			log::log("Ships:");
			for(auto& ship_iterator : me->ships)
				log::log(ship_iterator.second->to_string_ship());

			// Log all ships assigned to my shipyard
			log::log("Number of ships: " + to_string(my_ships_number()));

			// Log all predicted direction
			for (auto& ship_direction : moves_queue)
			{
				string log_moves = ship_direction.first->to_string_ship() + " moving to ";
				for (Direction& direction : ship_direction.second)
					log_moves += static_cast<char>(direction);
				
				log::log(log_moves);
			}
		
			// Log all commands
			for (auto& command: command_queue)
				log::log(command);

			log::log("Time taken: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));
			log::log("");
		}


		/* functions for new turn */
        void ready(const string& name, unsigned int rng_seed);
        void update_frame();
        bool end_turn(const vector<Command>& commands);
    };
}
