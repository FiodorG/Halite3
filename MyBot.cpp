#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"

#include <random>
#include <ctime>

using namespace std;
using namespace hlt;

int main(int argc, char* argv[]) 
{
    unsigned int rng_seed;
    if (argc > 1)
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
	else 
        rng_seed = static_cast<unsigned int>(time(nullptr));

	mt19937 rng(rng_seed);

    Game game;
    /* Warmup */
	/* End Warmup */
    game.ready("GSBot", rng_seed);

    while(true)
	{
		game.update_frame();
		game.log_start_turn();

        unique_ptr<GameMap>& game_map = game.game_map;
		shared_ptr<Shipyard> shipyard = game.me->shipyard;

		/* 
		Update SUICIDE_ON_BASE
		**********************
		*/
		for (const shared_ptr<Ship>& ship : game.me->ships_ordered)
		{
			if (ship->assigned)
				continue;

			if (2 * game_map->calculate_distance(ship->position, shipyard->position) >= game.turns_remaining())
				ship->assign_objective(Objective_Type::SUICIDE_ON_BASE, shipyard->position);

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

			if (ship->is_objective(Objective_Type::BACK_TO_BASE) && (ship->position == shipyard->position))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && ship->is_full(0.9))
				ship->assign_objective(Objective_Type::BACK_TO_BASE, shipyard->position);

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

			if (ship->is_objective(Objective_Type::EXTRACT) && (ship->position == ship->target_position()) && (game.better_neighboring_cell_exists(ship->position)))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && (game.enemy_in_adjacent_cell(ship->target_position())))
				ship->clear_objective();

			if(!ship->has_objective())
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
