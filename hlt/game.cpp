#include "game.hpp"
#include "input.hpp"

#include <sstream>
#include <ctime>
#include <stdint.h>
#include <algorithm>

using namespace std;

hlt::Game::Game(unordered_map<string, int> constants) :
	turn_number(0),
	constants(constants)
{
    std::ios_base::sync_with_stdio(false);

    hlt::constants::populate_constants(hlt::get_string());

    int num_players;
    std::stringstream input(get_string());
    input >> num_players >> my_id;

    log::open(my_id);

    for (int i = 0; i < num_players; ++i) 
        players.push_back(Player::_generate());
    
    me = players[my_id];
    game_map = GameMap::_generate();

	// My stuff
	number_of_players = players.size();
	total_ships_produced = 0;
	reserved_halite = 0;
	collision_resolver = CollisionResolver();
	scorer = Scorer(game_map->width, game_map->height);
	move_solver = MoveSolver();
	pathfinder = PathFinder(game_map->width);
	distance_manager = DistanceManager();
	objective_manager = ObjectiveManager();
	blocker = Blocker();

	distance_manager.closest_shipyard_or_dropoff = vector<vector<Position>>(game_map->height, vector<Position>(game_map->width, Position()));
	distance_manager.distance_cell_shipyard_or_dropoff = vector<vector<int>>(game_map->height, vector<int>(game_map->width, 0));
}

void hlt::Game::ready(const std::string& name, unsigned int rng_seed)
{
    std::cout << name << std::endl;
	log::log("Successfully created bot! My Player ID is " + to_string(my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");
}

void hlt::Game::update_frame() 
{
	start = clock();
    hlt::get_sstream() >> turn_number;
    log::log("=============== TURN " + std::to_string(turn_number) + " ================");

	// Any extra info in game, gamemap, mapcells, shipyard will stay over next turn

	// Update players: get new halite, recreate all new ships & dropoffs
    for (size_t i = 0; i < players.size(); ++i) 
	{
        PlayerId current_player_id;
        int num_ships;
        int num_dropoffs;
        Halite halite;
        hlt::get_sstream() >> current_player_id >> num_ships >> num_dropoffs >> halite;

        players[current_player_id]->_update(num_ships, num_dropoffs, halite);
    }

	// Reset halite on each cell and empty cells
    game_map->_update();

	// Add ships, shipyard and dropoffs to cells
    for (const auto& player : players) 
	{
        for (auto& ship_iterator : player->ships) 
		{
            auto ship = ship_iterator.second;
            game_map->at(ship)->mark_unsafe(ship);
        }

        game_map->at(player->shipyard)->structure = player->shipyard; // Shipyard never flushed

        for (auto& dropoff_iterator : player->dropoffs) 
		{
            auto dropoff = dropoff_iterator.second;
            game_map->at(dropoff)->structure = dropoff;
        }
    }

	command_queue.clear();

	// Navigation
	positions_next_turn.clear();

	// Objectives
	reserved_halite = 0;
	objective_manager.turn_since_last_dropoff++;
	objective_manager.flush_objectives();

	// Scorer
	scorer.update_grids(*this);
	scorer.halite_total = 0;
	for (vector<MapCell>& row : game_map->cells)
		for (MapCell& cell : row)
			scorer.halite_total += cell.halite;
	if (turn_number <= 1)
		scorer.halite_initial = scorer.halite_total;

	scorer.halite_percentile = 0;
	int i = 0;
	vector<int> halite_all = vector<int>(game_map->width * game_map->height, 0);
	for (vector<MapCell>& row : game_map->cells)
		for (MapCell& cell : row)
			halite_all[i++] = cell.halite;
	std::sort(halite_all.begin(), halite_all.end());
	scorer.halite_percentile = halite_all[(int)(0.5 * game_map->width * game_map->height)];

	// Distance manager
	distance_manager.fill_closest_shipyard_or_dropoff(*this);

	// Blocker
	blocker.fill_positions_to_block_scores(*this);
}

bool hlt::Game::end_turn(const std::vector<hlt::Command>& commands) 
{
    for (const auto& command : commands) 
	{
        std::cout << command << ' ';
    }
    std::cout << std::endl;
    return std::cout.good();
}

void hlt::Game::fudge_ship_if_base_blocked()
{
	if ((turn_number < 20) && (me->ships.size() <= 5))
	{
		int all_distance_from_shipyard = 0;
		for (auto& ship_iterator : me->ships)
			if (distance(ship_iterator.second->position, my_shipyard_position()) == 1)
				all_distance_from_shipyard += 1;

		if (
			(all_distance_from_shipyard == 4) &&
			game_map->at(my_shipyard_position())->is_occupied()
			)
		{
			shared_ptr<Ship> ship_with_least_halite;
			int min_halite = 9999999;

			for (auto& ship_iterator : me->ships)
			{
				if (
					(game_map->at(ship_iterator.second)->halite < min_halite) &&
					(ship_iterator.second->position != my_shipyard_position()) &&
					ship_can_move(ship_iterator.second)
					)
				{
					ship_with_least_halite = ship_iterator.second;
					min_halite = game_map->at(ship_iterator.second)->halite;
				}
			}

			// Move ship away from shipyard
			Direction direction = invert_direction(game_map->get_move(ship_with_least_halite->position, my_shipyard_position()));
			Position position = game_map->directional_offset(ship_with_least_halite->position, direction);
			assign_objective(ship_with_least_halite, Objective_Type::EXTRACT, position);
			update_ship_target_position(ship_with_least_halite, position);
			
			log::log("Pushing " + ship_with_least_halite->to_string_ship());

			// Ship on shipyard sent to ship moved away
			assign_objective(ship_on_shipyard(), Objective_Type::EXTRACT, ship_with_least_halite->position);
			update_ship_target_position(ship_on_shipyard(), ship_with_least_halite->position);

			log::log("Pushing " + ship_on_shipyard()->to_string_ship());
		}
	}
}
