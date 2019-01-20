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

			{
				Stopwatch s("Generate moves");

				for (auto& ship : game.objective_manager.ships_ordered)
				{
					pair<Position, double> action = game.move_solver.find_best_action(ship, game);
					game.assign_ship_to_target_position(ship, action.first);
				}
			}

			//if (game.turn_number == 100) exit(1);

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
