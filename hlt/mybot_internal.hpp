#include "game.hpp"
#include "constants.hpp"
#include "log.hpp"

#include <climits>
#include <cfloat>

using namespace std;
using namespace hlt;

namespace hlt
{
	int mybot_internal(string bot_name, unordered_map<string, int> constants, unsigned int rng_seed)
	{
		Game game(constants);
		/* Warmup */
		/* End Warmup */
		game.ready(bot_name, rng_seed);

		while (true)
		{
			game.update_frame();
			game.log_start_turn();

			/*
			Update MAKE DROPOFF
			**********************
			*/
			for (const shared_ptr<Ship>& ship : game.me->my_ships)
			{
				if (ship->assigned)
					continue;

				if (game.should_spawn_dropoff(ship))
				{
					ship->assign_objective(Objective_Type::MAKE_DROPOFF, ship->position);
					ship->set_assigned();
				}
			}

			/*
			Update SUICIDE_ON_BASE
			**********************
			*/
			for (const shared_ptr<Ship>& ship : game.me->my_ships)
			{
				if (ship->assigned)
					continue;

				if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE) || (2 * game.game_map->calculate_distance(ship->position, game.get_closest_shipyard_or_dropoff(ship)) >= game.turns_remaining()))
				{
					ship->assign_objective(Objective_Type::SUICIDE_ON_BASE, game.get_closest_shipyard_or_dropoff(ship));
					ship->set_assigned();
				}
			}

			/*
			Update RETURN_TO_BASE
			**********************
			*/
			for (const shared_ptr<Ship>& ship : game.me->my_ships)
			{
				if (ship->assigned)
					continue;

				if (ship->is_objective(Objective_Type::BACK_TO_BASE) && (game.is_shipyard_or_dropoff(ship->position)))
					ship->clear_objective();

				if ((ship->is_objective(Objective_Type::BACK_TO_BASE)) || (ship->is_full(0.9)))
				{
					ship->assign_objective(Objective_Type::BACK_TO_BASE, game.get_closest_shipyard_or_dropoff(ship));
					ship->set_assigned();
				}
			}

			/*
			Update EXTRACT_ZONE 
			**********************
			In globally greedy assignment
			*/
			unordered_map<shared_ptr<Ship>, double> ships_without_objectives;
			for (const shared_ptr<Ship>& ship : game.me->my_ships)
				if(!ship->assigned)
					ships_without_objectives[ship] = -DBL_MAX;

			while (ships_without_objectives.size())
			{
				shared_ptr<Ship> best_ship;
				MapCell* best_cell = &game.game_map->cells[0][0];
				double best_score = -DBL_MAX;

				for (const auto& ship : ships_without_objectives)
				{
					pair<MapCell*, double> action = game.scorer.find_best_objective_cell(ship.first, game);

					if (action.second > best_score)
					{
						best_ship = ship.first;
						best_score = action.second;
						best_cell = action.first;
					}
				}

				log::log(best_ship->to_string_ship() + " assigned to area " + best_cell->position.to_string_position() + " with score " + to_string(best_score));

				game.scorer.decreases_score_in_target_area(best_ship, best_cell, game);
				best_ship->assign_objective(Objective_Type::EXTRACT_ZONE, best_cell->position, best_score);
				ships_without_objectives.erase(best_ship);
			}

			/*
			Compute ship moves
			**********************
			In globally greedy assignment
			*/
			unordered_map<shared_ptr<Ship>, int> ships_without_actions;
			for (const shared_ptr<Ship>& ship : game.me->my_ships)
				ships_without_actions[ship] = INT_MIN;

			while (ships_without_actions.size())
			{
				shared_ptr<Ship> best_ship;
				Position best_position;
				int best_score = INT_MIN;

				vector<shared_ptr<Ship>> filtered_ships = game.move_solver.filter_ships_without_actions(ships_without_actions);
				for (const auto& ship : filtered_ships)
				{
					pair<Position, int> action = game.move_solver.find_best_action(ship, game);

					if (action.second > best_score)
					{
						best_ship     = ship;
						best_score    = action.second;
						best_position = action.first;
					}
				}

				log::log(best_ship->to_string_ship() + " goes to " + best_position.to_string_position() + " with score " + to_string(best_score));

				game.assign_ship_to_target_position(best_ship, best_position);
				ships_without_actions.erase(best_ship);
			}

			//if (game.turn_number == 11)
			//	exit(1);

			game.fudge_ship_if_base_blocked();
			game.resolve_moves();

			game.generate_new_ships();
			game.log_end_turn();

			if (!game.end_turn(game.command_queue))
				break;
		}

		return 0;
	}
}
