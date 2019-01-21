#pragma once

#include "game_map.hpp"
#include "player.hpp"
#include "types.hpp"
#include "collision_resolver.hpp"
#include "pathfinder.hpp"
#include "log.hpp"
#include "scorer.hpp"
#include "move_solver.hpp"
#include "blocker.hpp"
#include "distance_manager.hpp"
#include "objective_manager.hpp"
#include "defines.hpp"
#include "stopwatch.hpp"

#include <vector>
#include <iostream>
#include <cmath>
#include <math.h>
#include <utility>
#include <cfloat>
#include <climits>

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
		int reserved_halite;
		int number_of_players;
		int total_ships_produced;

		// For timer
		clock_t start;

		// Movement management
		CollisionResolver collision_resolver;
		PathFinder pathfinder;
		
		unordered_map<shared_ptr<Ship>, Position> positions_next_turn;
		vector<Command> command_queue;

		// Scoring
		Scorer scorer;
		DistanceManager distance_manager;

		// Move Solver
		MoveSolver move_solver;

		// Objectives
		ObjectiveManager objective_manager;

		// Blocker
		Blocker blocker;

		Game(unordered_map<string, int> constants);

		void resolve_moves()
		{
			Stopwatch s("Collising Resolve");
			command_queue = collision_resolver.resolve_moves(*this);
		}

		void assign_ship_to_target_position(shared_ptr<Ship> ship, const Position& position)
		{
			ship->set_assigned();

			if (ship_can_move(ship))
			{
				Position target_position = pathfinder.compute_path(ship, position, *this);
				update_ship_target_position(ship, target_position);
			}
			else
			{
				update_ship_target_position(ship, ship->position);
			}
		}

		void update_ship_target_position(shared_ptr<Ship> ship, const Position& position)
		{
			// to remove this
			if (positions_next_turn.count(ship))
				scorer.flush_grid_score(positions_next_turn[ship]);

			positions_next_turn[ship] = position;
			scorer.add_self_ships_to_grid_score(ship, position);
		}

		bool enough_halite_per_ship()
		{
			double limit;

			switch (game_map->width)
			{
			case 32:
				limit = 200.0;
				break;
			case 40:
				limit = 300.0;
				break;
			case 48:
				limit = 300.0;
				break;
			case 56:
				limit = 400.0;
				break;
			case 64:
				limit = 400.0;
				break;
			default:
				log::log("Unknown map width");
				exit(1);
			}

			return ((double)scorer.halite_total / (double)total_ships_number() >= limit);
		}

		void generate_new_ships()
		{
			if (players.size() == 2)
			{
				if (
					turns_remaining_percent() >= 0.33 &&
					me->halite - reserved_halite >= constants::SHIP_COST &&
					!position_occupied_next_turn(my_shipyard_position()) &&
					my_ships_number() <= max_allowed_ships()
					)
				{
					command_queue.push_back(me->shipyard->spawn());
					total_ships_produced++;
				}
			}
			else
			{
				if (
					turns_remaining_percent() >= 0.40 &&
					me->halite - reserved_halite >= constants::SHIP_COST &&
					!position_occupied_next_turn(my_shipyard_position()) &&
					my_ships_number() <= max_allowed_ships() &&
					enough_halite_per_ship()
					)
				{
					command_queue.push_back(me->shipyard->spawn());
					total_ships_produced++;
				}
			}
		}

		int max_allowed_ships() const
		{
			int max_allowed_ships = 0;

			if (players.size() == 2)
			{
				max_allowed_ships = min(150, (int)(20.0 + 0.00015 * (double)scorer.halite_initial));
			}
			else if (players.size() == 4)
			{
				max_allowed_ships = min(150, (int)(20.0 + 0.0001 * (double)scorer.halite_initial));

				if (game_map->width == 32)
					max_allowed_ships = (int)(0.9 * max_allowed_ships) + 1;

				if (game_map->width == 40)
					max_allowed_ships = (int)(0.95 * max_allowed_ships) + 1;
			} 
			else
			{
				log::log("Unknown number of players");
				exit(1);
			}

			return max_allowed_ships;
		}

		void fudge_ship_if_base_blocked();


		/* Utilities */
		inline MapCell* mapcell(const Position& position) const { return game_map->at(position); }
		inline MapCell* mapcell(shared_ptr<Ship> ship) const { return game_map->at(ship->position); }
		inline MapCell* mapcell(int x, int y) const { return game_map->at(y, x); }
		inline bool enemy_in_cell(const MapCell& cell) const { return cell.is_occupied_by_enemy(my_id); }
		inline bool enemy_in_cell(const Position& position) const { return game_map->at(position)->is_occupied_by_enemy(my_id); }
		inline bool enemy_dropoff_in_cell(const Position& position) const { return game_map->at(position)->has_structure() && (game_map->at(position)->structure->owner != my_id); }
		bool enemy_in_adjacent_cell(const Position& position) const
		{
			return
				game_map->at(game_map->directional_offset(position, Direction::NORTH))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::SOUTH))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::EAST))->is_occupied_by_enemy(my_id) ||
				game_map->at(game_map->directional_offset(position, Direction::WEST))->is_occupied_by_enemy(my_id) ||
				game_map->at(position)->is_occupied_by_enemy(my_id);
		}
		inline bool ally_in_cell(const Position& position) const { return game_map->at(position)->is_occupied_by_ally(my_id); }
		inline int halite_on_position(const Position& position) const { return mapcell(position)->halite; }
		inline bool position_has_ship(const Position& position) const { return mapcell(position)->is_occupied(); }
		inline shared_ptr<Ship> ship_on_position(const Position& position) const { return mapcell(position)->ship; }
		PlayerId playerid_on_position(const Position& position) const { return mapcell(position)->ship->owner; }
		vector<shared_ptr<Ship>> enemies_adjacent_to_position(const Position& position) const
		{
			vector<shared_ptr<Ship>> enemies;

			if (game_map->at(game_map->directional_offset(position, Direction::NORTH))->is_occupied_by_enemy(my_id))
				enemies.push_back(ship_on_position(game_map->directional_offset(position, Direction::NORTH)));

			if (game_map->at(game_map->directional_offset(position, Direction::SOUTH))->is_occupied_by_enemy(my_id))
				enemies.push_back(ship_on_position(game_map->directional_offset(position, Direction::SOUTH)));

			if (game_map->at(game_map->directional_offset(position, Direction::EAST))->is_occupied_by_enemy(my_id))
				enemies.push_back(ship_on_position(game_map->directional_offset(position, Direction::EAST)));

			if (game_map->at(game_map->directional_offset(position, Direction::WEST))->is_occupied_by_enemy(my_id))
				enemies.push_back(ship_on_position(game_map->directional_offset(position, Direction::WEST)));

			if (game_map->at(position)->is_occupied_by_enemy(my_id))
				enemies.push_back(ship_on_position(position));

			return enemies;
		}
		vector<Position> adjacent_positions_to_position(const Position& position) const
		{
			vector<Position> positions;

			positions.push_back(game_map->directional_offset(position, Direction::NORTH));
			positions.push_back(game_map->directional_offset(position, Direction::SOUTH));
			positions.push_back(game_map->directional_offset(position, Direction::EAST));
			positions.push_back(game_map->directional_offset(position, Direction::WEST));

			return positions;
		}
		vector<Position> nearby_positions_to_position(const Position& position, int d) const
		{
			vector<Position> positions;

			int width = game_map->width;
			int height = game_map->height;

			int target_x = position.x;
			int target_y = position.y;

			for (int i = 0; i <= d * 2; ++i)
				for (int j = 0; j <= d * 2; ++j)
				{
					int new_x = (((target_x - d + i) % width) + width) % width;
					int new_y = (((target_y - d + j) % height) + height) % height;
					Position new_position = Position(new_x, new_y);

					if (distance(position, new_position) <= d)
						positions.push_back(new_position);
				}

			return positions;
		}

		inline int get_constant(string name) const { return constants.at(name); }
		inline double turn_percent() const { return (double)turn_number / (double)constants::MAX_TURNS; }
		inline double turns_remaining_percent() const { return (double)(constants::MAX_TURNS - turn_number) / (double)constants::MAX_TURNS; }
		inline int turns_remaining() const { return constants::MAX_TURNS - turn_number; }
		inline int my_ships_number() const { return me->ships.size(); }
		int total_ships_number() const 
		{ 
			int n = 0;
			for (const auto& player : players)
				n += player->ships.size();
			return n;
		}
		inline int my_dropoff_number() const { return me->dropoffs.size(); }
		inline bool is_two_player_game() const { return number_of_players == 2; }
		inline bool is_four_player_game() const { return number_of_players == 4; }

		Position my_shipyard_position() const { return me->shipyard->position; }
		vector<Position> my_shipyard_or_dropoff_positions() const
		{
			vector<Position> positions;
			positions.push_back(my_shipyard_position());
			for (auto& dropoff : me->dropoffs)
				positions.push_back(dropoff.second->position);
			return positions;
		}
		vector<Position> enemy_shipyard_or_dropoff_positions() const
		{
			vector<Position> positions;
			for (const auto& player : players)
			{
				if (player->id == my_id)
					continue;

				positions.push_back(player->shipyard->position);
				for (auto& dropoff : player->dropoffs)
					positions.push_back(dropoff.second->position);
			}
			
			return positions;
		}
		shared_ptr<Ship> ship_on_shipyard() const { return game_map->at(my_shipyard_position())->ship; }
		Position get_closest_shipyard_or_dropoff(shared_ptr<Ship> ship, bool with_fakes = true) const
		{
			return get_closest_shipyard_or_dropoff(ship->position, with_fakes);
		}
		Position get_closest_shipyard_or_dropoff(const Position& position, bool with_fakes = true) const
		{
			int min_distance = distance(position, my_shipyard_position());
			Position closest_shipyard = my_shipyard_position();

			for (auto& dropoff_iterator : me->dropoffs)
			{
				if ((!with_fakes) && (dropoff_iterator.second->fake))
					continue;

				int dist = distance(position, dropoff_iterator.second->position);

				if (dist <= min_distance)
				{
					min_distance = dist;
					closest_shipyard = dropoff_iterator.second->position;
				}
			}

			return closest_shipyard;
		}
		Position get_closest_enemy_shipyard_or_dropoff(const Position& position) const
		{
			int min_distance = INT_MAX;
			Position closest_shipyard = my_shipyard_position();
			vector<Position> enemy_positions = enemy_shipyard_or_dropoff_positions();

			for (auto& enemy_position : enemy_positions)
			{
				int distance = game_map->calculate_distance(position, enemy_position);

				if (distance <= min_distance)
				{
					min_distance = distance;
					closest_shipyard = enemy_position;
				}
			}

			return closest_shipyard;
		}
		Position get_closest_enemy_shipyard_or_dropoff(shared_ptr<Ship> ship) const { return get_closest_enemy_shipyard_or_dropoff(ship->position); }
		bool is_shipyard_or_dropoff(const Position& position) const { return mapcell(position)->is_shipyard_or_dropoff(my_id); }
		bool position_occupied_next_turn(const Position& position) const
		{
			for (auto& ship_position : positions_next_turn)
				if (ship_position.second == position)
					return true;

			return false;
		}
		bool close_to_crowded_area(const Position& position, int d) const
		{
			Position upper_left = Position(0, 0);
			Position upper_right = Position((int)(game_map->width / 2.0), 0);
			Position lower_left = Position(0, (int)(game_map->height / 2.0));
			Position lower_right = Position((int)(game_map->width / 2.0), (int)(game_map->height / 2.0));

			if (
				(distance(position, upper_left) <= d) ||
				(distance(position, upper_right) <= d) ||
				(distance(position, lower_left) <= d) ||
				(distance(position, lower_right) <= d)
				)
				return true;
			else
				return false;
		}
		bool close_to_axis(const Position& position, int d) const
		{
			int width = game_map->width;

			if (
				(position.x <= d) ||
				(position.y <= d) ||
				(width - position.y <= d) ||
				(width - position.x <= d) ||
				(abs(position.x - width / 2) <= d) ||
				(abs(position.y - width / 2) <= d)
				)
				return true;
			else
				return false;
		}

		inline int distance(const Position& position1, const Position& position2) const {  return game_map->calculate_distance(position1, position2); }
		int distance_from_objective(shared_ptr<Ship> ship) const { return game_map->calculate_distance(ship->position, ship->target_position()); }
		bool ship_can_move(shared_ptr<Ship> ship) const { return (mapcell(ship)->halite < 10) || (ship->halite >= (int)floor(0.1 * mapcell(ship)->halite)); }
		shared_ptr<Ship> closest_ship_to_position(const Position& position) const
		{
			shared_ptr<Ship> closest_ship = me->my_ships[0];
			int closest_distance = 9999;

			for (const shared_ptr<Ship>& ship : me->my_ships)
			{
				int current_distance = distance(ship->position, position);
				if (current_distance < closest_distance)
				{
					closest_ship = ship;
					closest_distance = current_distance;
				}
			}

			return closest_ship;
		}
		inline bool switch_to_half_full_for_rtb(shared_ptr<Ship> ship) const
		{
			double limit = 23000.0 * (double)game_map->width * (double)game_map->width / 1024.0;

			return is_four_player_game() &&
				(turns_remaining_percent() <= 0.5) &&
				(scorer.halite_total <= (int)limit) &&
				(scorer.get_grid_score_extract_nearby(ship->position) <= 300);
		}

		void assign_objective(shared_ptr<Ship> ship, Objective_Type objective_type, const Position& position, double score = 0)
		{
			if (score == 0.0)
				score = distance(ship->position, position);

			ship->objective = shared_ptr<Objective>(new Objective(0, objective_type, position, score));
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
			log::log("Total ships: " + to_string(my_ships_number()));
			log::log("Total halite: " + to_string(scorer.halite_total));
			log::log("50th pctl halite: " + to_string(scorer.halite_percentile));
		
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
