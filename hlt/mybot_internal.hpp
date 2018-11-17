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
			{
				Stopwatch s("Generate moves");

				for (int i = 0; i < 1; i++)
				{
					for (auto& ship : game.objective_manager.ships_ordered)
					{
						pair<Position, double> action = game.move_solver.find_best_action(ship, game);
						game.assign_ship_to_target_position(ship, action.first);
					}
				}
			}

			//int shipid = 3;
			//if (game.me->ships.count(shipid))
			//	game.scorer.find_best_objective_cell(game.me->ships[shipid], game, (game.turn_number == 13));

			//shared_ptr<Ship> ship = make_shared<Ship>(Ship(game.my_id, 0, 10, 24, 3));
			//game.assign_objective(ship, Objective_Type::EXTRACT_ZONE, ship->position);

			//game.move_solver.score_path2(
			//	ship,
			//	vector<Direction>{Direction::STILL, Direction::WEST, Direction::STILL, Direction::STILL, Direction::STILL},
			//	5,
			//	game
			//	);
			//exit(1);

			//if (game.turn_number == 65)
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
