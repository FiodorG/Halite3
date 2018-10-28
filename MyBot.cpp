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
    game.ready("MyCppBot", rng_seed);

    while(true)
	{
		game.update_frame();
		game.log_start_turn();

        unique_ptr<GameMap>& game_map = game.game_map;
		shared_ptr<Shipyard> shipyard = game.me->shipyard;

		/* 
		Update SUICIDE_ON_BASE
		**********************
		No ordering here, all RTBing ships are equal
		*/
		for (const auto& ship_iterator : game.me->ships)
		{
			shared_ptr<Ship> ship = ship_iterator.second;

			if (ship->assigned)
				continue;

			if (2 * game_map->calculate_distance(ship->position, shipyard->position) >= game.turns_remaining())
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::SUICIDE_ON_BASE, shipyard->position)));

			if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
				game.assign_ship_to_target_position(ship);
		}

		/* 
		Update RETURN_TO_BASE
		**********************
		No ordering here, all RTBing ships are equal
		*/
		for (const auto& ship_iterator : game.me->ships)
		{
			shared_ptr<Ship> ship = ship_iterator.second;

			if (ship->assigned)
				continue;

			if (ship->is_objective(Objective_Type::BACK_TO_BASE) && (ship->position == shipyard->position))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && ship->is_full(0.9))
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::BACK_TO_BASE, shipyard->position)));

			if (ship->is_objective(Objective_Type::BACK_TO_BASE))
				game.assign_ship_to_target_position(ship);
		}

		/*
		Update EXTRACT
		**********************
		Ordering in halite cargo - fullest ships have priority (todo)
		*/
        for (const auto& ship_iterator : game.me->ships)
		{
            shared_ptr<Ship> ship = ship_iterator.second;

			if (ship->assigned)
				continue;

			if (ship->is_objective(Objective_Type::EXTRACT) && (ship->position == ship->target_position()) && (game_map->at(ship->target_position())->halite < 40))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && (game.enemy_in_adjacent_cell(ship->target_position())))
				ship->clear_objective();

			if(!ship->has_objective())
			{
				MapCell* target_position = game_map->closest_cell_with_ressource(ship, game);
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::EXTRACT, target_position->position)));
			}

			if (ship->is_objective(Objective_Type::EXTRACT))
				game.assign_ship_to_target_position(ship);
        }

		game.fudge_ship_if_base_blocked();
		game.resolve_moves();

        if (
			game.turns_remaining_percent() >= 0.25 &&
			game.me->halite >= constants::SHIP_COST &&
			!game.position_occupied_next_turn(game.my_shipyard_position()) &&
			game.my_ships_number() < game.max_allowed_ships()
		)
			game.command_queue.push_back(game.me->shipyard->spawn());
			
		game.log_end_turn();

        if (!game.end_turn(game.command_queue))
            break;
    }

    return 0;
}
