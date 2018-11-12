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

			//int shipid = 3;
			//if (game.me->ships.count(shipid))
			//	game.scorer.find_best_objective_cell(game.me->ships[shipid], game, (game.turn_number == 13));

			//if (game.turn_number == 200)
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
