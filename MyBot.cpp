#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"

#include <random>
#include <unordered_map>
#include <ctime>

using namespace std;
using namespace hlt;

int mybot_internal(Game& game, unsigned int rng_seed)
{
	/* Warmup */
	/* End Warmup */
	game.ready("GSBot", rng_seed);

	while (true)
	{
		game.update_frame();
		game.log_start_turn();

		unique_ptr<GameMap>& game_map = game.game_map;

		/*
		Update MAKE DROPOFF
		**********************
		*/
		for (const shared_ptr<Ship>& ship : game.me->ships_ordered)
		{
			if (ship->assigned)
				continue;

			if (game.should_spawn_dropoff(ship))
				ship->assign_objective(Objective_Type::MAKE_DROPOFF, ship->position);

			if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
			{
				game.assign_ship_to_target_position(ship);
				break;
			}
		}

		/*
		Update SUICIDE_ON_BASE
		**********************
		*/
		for (const shared_ptr<Ship>& ship : game.me->ships_ordered)
		{
			if (ship->assigned)
				continue;

			if (2 * game_map->calculate_distance(ship->position, game.get_closest_shipyard_or_dropoff(ship)) >= game.turns_remaining())
				ship->assign_objective(Objective_Type::SUICIDE_ON_BASE, game.get_closest_shipyard_or_dropoff(ship));

			if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
				game.assign_ship_to_target_position(ship);
		}

		/*
		Update RETURN_TO_BASE
		**********************
		*/
		for (const shared_ptr<Ship>& ship : game.me->ships_ordered)
		{
			if (ship->assigned)
				continue;

			if (ship->is_objective(Objective_Type::BACK_TO_BASE) && (game.is_shipyard_or_dropoff(ship->position)))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::BACK_TO_BASE))
				ship->assign_objective(Objective_Type::BACK_TO_BASE, game.get_closest_shipyard_or_dropoff(ship));

			if (ship->is_objective(Objective_Type::EXTRACT) && ship->is_full(0.9))
				ship->assign_objective(Objective_Type::BACK_TO_BASE, game.get_closest_shipyard_or_dropoff(ship));

			if (ship->is_objective(Objective_Type::BACK_TO_BASE))
				game.assign_ship_to_target_position(ship);
		}

		/*
		Update EXTRACT
		**********************
		Ordering in halite cargo - fullest ships have priority (todo)
		*/
		for (const shared_ptr<Ship>& ship : game.me->ships_ordered)
		{
			if (ship->assigned)
				continue;

			if (ship->is_objective(Objective_Type::EXTRACT) && (ship->position == ship->target_position()) && (game.game_map->better_neighboring_cell_exists(ship->position)))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && (game.enemy_in_adjacent_cell(ship->target_position())))
				ship->clear_objective();

			if (!ship->has_objective())
			{
				MapCell* target_position = game_map->closest_cell_with_ressource(ship, game);
				ship->assign_objective(Objective_Type::EXTRACT, target_position->position);
			}

			if (ship->is_objective(Objective_Type::EXTRACT))
				game.assign_ship_to_target_position(ship);
		}

		game.fudge_ship_if_base_blocked();
		game.resolve_moves();

		game.generate_new_ships();
		game.log_end_turn();

		if (!game.end_turn(game.command_queue))
			break;
	}

	return 0;
}

int main(int argc, char* argv[]) 
{
    unsigned int rng_seed;
    if (argc > 1)
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
	else 
        rng_seed = static_cast<unsigned int>(time(nullptr));
	mt19937 rng(rng_seed);

	/* Constants */
	unordered_map<string, int> constants;
	constants["A* Heuristic"] = 100;

	constants["Dropoff: halite nearby"] = 3500;
	constants["Dropoff: distance from shipyard"] = 14;
	constants["Dropoff: distance nearby"] = 5;

    Game game(constants);

	return mybot_internal(game, rng_seed);
}
