#pragma once

#include "game_map.hpp"
#include "player.hpp"
#include "types.hpp"
#include "navigation_manager.hpp"
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

		unordered_map<shared_ptr<Ship>, vector<Direction>> moves_queue;
		vector<Command> command_queue;

		unique_ptr<NavigationManager> navigation_manager;

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

		int max_allowed_ships() const
		{
			switch (game_map->width)
			{
				case 32:
					return 25;
				case 40:
					return 29;
				case 48:
					return 33;
				case 56:
					return 37;
				case 64:
					return 40;
				default:
					log::log("Unknown map width");
					exit(1);
			}
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
			log::log("Ships assigned to shipyard: " + to_string(me->shipyard->n_assigned_ships));

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

			log::log("");
		}


		/* functions for new turn */
        void ready(const string& name, unsigned int rng_seed);
        void update_frame();
        bool end_turn(const vector<Command>& commands);
    };
}
