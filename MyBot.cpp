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

    for (;;) 
	{
		game.update_frame();
		game.log_start_turn();

        unique_ptr<GameMap>& game_map = game.game_map;
		shared_ptr<Shipyard> shipyard = game.me->shipyard;

        for (const auto& ship_iterator : game.me->ships)
		{
            shared_ptr<Ship> ship = ship_iterator.second;

			shipyard->assign_ship(ship);

			// Update objectives from last turn
			if (2 * game_map->calculate_distance(ship->position, shipyard->position) > game.turns_remaining())
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::SUICIDE_ON_BASE, shipyard->position)));
			
			if (ship->is_objective(Objective_Type::BACK_TO_BASE) && (ship->position == shipyard->position))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && (ship->position == ship->target_position()) && (game_map->at(ship->target_position())->halite < 40))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && (game.enemy_in_cell(ship->target_position())))
				ship->clear_objective();

			if (ship->is_objective(Objective_Type::EXTRACT) && ship->is_full())
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::BACK_TO_BASE, shipyard->position)));

			// Act on existing objectives
            if (ship->has_objective())
			{
				game.moves_queue[ship] = game_map->navigate(ship, ship->target_position());
			}
			// Or create new one
			else
			{
				MapCell* target_position = game_map->closest_cell_with_ressource(*ship, game);
				ship->assign_objective(shared_ptr<Objective>(new Objective(0, Objective_Type::EXTRACT, target_position->position)));

				game.moves_queue[ship] = game_map->navigate(ship, target_position->position);
			}
        }

		game.resolve_moves();

        if (
			game.turns_remaining() >= 40 &&
			game.me->halite >= constants::SHIP_COST &&
			!game.navigation_manager->shipyard_occupied_next_turn(*game.me->shipyard) && 
			shipyard->n_assigned_ships < game.max_allowed_ships()
		)
			game.command_queue.push_back(game.me->shipyard->spawn());
			
		game.log_end_turn();

        if (!game.end_turn(game.command_queue))
            break;
    }

    return 0;
}
