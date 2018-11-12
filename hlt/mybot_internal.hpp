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

			game.objective_manager.assign_objectives(game);
			game.objective_manager.get_ordered_ships(game);

			/*
			Compute ship moves
			**********************
			*/
			for (int i = 0; i < 1; i++)
			{
				for (auto& ship : game.objective_manager.ships_ordered)
				{
					pair<Position, int> action = game.move_solver.find_best_action(ship, game);

					ship->set_objective_position(action.first);
					game.assign_ship_to_target_position(ship, action.first);
					log::log(ship->to_string_ship() + " goes to " + action.first.to_string_position());
				}
			}

			//auto ships_without_actions = game.move_solver.split_ships_without_actions(game);

			//while (game.move_solver.ships_without_actions_size(ships_without_actions))
			//{
			//	shared_ptr<Ship> best_ship;
			//	Position best_position;
			//	int best_score = INT_MIN;

			//	vector<shared_ptr<Ship>> filtered_ships = game.move_solver.get_ships_without_actions(ships_without_actions);
			//	for (const auto& ship : filtered_ships)
			//	{
			//		pair<Position, int> action = game.move_solver.find_best_action(ship, game);

			//		if (action.second > best_score)
			//		{
			//			best_ship     = ship;
			//			best_score    = action.second;
			//			best_position = action.first;
			//		}
			//	}

			//	//best_ship->set_objective_position(best_position);
			//	game.assign_ship_to_target_position(best_ship, best_position);

			//	log::log(best_ship->to_string_ship() + " goes to " + best_position.to_string_position() + " with score " + to_string(best_score));

			//	game.move_solver.remove_ship_from_available(ships_without_actions, best_ship);
			//}

			//int shipid = 3;
			//if (game.me->ships.count(shipid))
			//	game.scorer.find_best_objective_cell(game.me->ships[shipid], game, (game.turn_number == 13));

			//if (game.turn_number == 100)
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
