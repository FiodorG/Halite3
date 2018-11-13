#include "objective_manager.hpp"
#include "game.hpp"

using namespace hlt;
using namespace std;


int ObjectiveManager::max_allowed_dropoffs(const Game& game) const
{
	int max_dropoffs = 0;
	switch (game.game_map->width)
	{
	case 32:
		max_dropoffs = (game.players.size() == 2) ? 1 : 0;
		break;
	case 40:
		max_dropoffs = 1;
		break;
	case 48:
		max_dropoffs = 2;
		break;
	case 56:
		max_dropoffs = (game.players.size() == 2) ? 3 : 2;
		break;
	case 64:
		max_dropoffs = (game.players.size() == 2) ? 3 : 2;
		break;
	default:
		log::log("Unknown map width");
		exit(1);
	}

	// dropoff per tranche of 100k above 100k, capped at value above
	int base_dropoffs = min((int)(max((double)game.scorer.halite_initial - 100000.0, 0.0) / 100000.0), max_dropoffs);

	return base_dropoffs;
}

vector<Objective> ObjectiveManager::create_dropoff_objectives(const Game& game)
{
	//log::log("Turn since last:" + to_string(turn_since_last_dropoff));
	//log::log("Dropoff number: " + to_string(game.my_dropoff_number()) + ", allowed: " + to_string(game.max_allowed_dropoffs()));
	//for (auto& dropoff : game.me->dropoffs)
	//	log::log(dropoff.second->to_string_dropoff());

	if (!should_spawn_dropoff(game, vector<Objective>()))
		return vector<Objective>();

	vector<Position> dropoffs;
	dropoffs.push_back(game.my_shipyard_position());
	for (auto& dropoff : game.me->dropoffs)
		dropoffs.push_back(dropoff.second->position);

	pair<MapCell*, double> action = game.scorer.find_best_dropoff_cell(game.me->shipyard, dropoffs, game);

	log::log("Dropoff objective: " + action.first->position.to_string_position() + " with score " + to_string(action.second));

	return vector<Objective>{Objective(0, Objective_Type::MAKE_DROPOFF, action.first->position)};
}

bool ObjectiveManager::should_spawn_dropoff(const Game& game, vector<Objective> objectives_dropoffs)
{
	if (game.my_dropoff_number() >= max_allowed_dropoffs(game))
		return false;

	if (game.my_dropoff_number() == 0)
		return (game.my_ships_number() >= 20);
	else if (game.my_dropoff_number() == 1)
		return (game.my_ships_number() >= 40);
	else if (game.my_dropoff_number() == 2)
		return (game.my_ships_number() >= 40) && (turn_since_last_dropoff >= 80);

	return false;
}

bool ObjectiveManager::can_spawn_dropoff(const shared_ptr<Ship> ship, Game& game)
{
	if (
		ship->is_objective(Objective_Type::MAKE_DROPOFF) &&
		(game.distance_from_objective(ship) <= 2)  &&
		(game.me->halite + ship->halite + game.mapcell(ship)->halite >= 5 + constants::DROPOFF_COST) && 
		!game.mapcell(ship->position)->has_structure()
		)
	{
		game.objective_manager.turn_since_last_dropoff = 0;
		return true;
	}

	return false;
}

void ObjectiveManager::assign_objectives(Game& game)
{
	/*
	Update MAKE DROPOFF
	**********************
	*/
	{
		Stopwatch s("Dropoffs");
		log::log("Max allowed dropoff: " + to_string(max_allowed_dropoffs(game)));

		vector<Objective> objectives_dropoffs = create_dropoff_objectives(game);
		int objective_id = -1;
		for (Objective& dropoff_objective : objectives_dropoffs)
		{
			shared_ptr<Ship> closest_ship = game.closest_ship_to_position(dropoff_objective.target_position);
			game.assign_objective(closest_ship, Objective_Type::MAKE_DROPOFF, dropoff_objective.target_position);
			closest_ship->set_assigned();

			if (game.distance_from_objective(closest_ship) <= 10)
			{
				game.reserved_halite += constants::DROPOFF_COST - closest_ship->halite - game.halite_on_position(closest_ship->position);
				log::log(closest_ship->to_string_ship() + " is close to target dropoff, reserved halite " + to_string(game.reserved_halite));
			}

			if ((game.distance_from_objective(closest_ship) <= 5) && (game.me->halite + closest_ship->halite + game.halite_on_position(closest_ship->position) >= 3000))
			{
				bool is_fake = (game.me->halite + closest_ship->halite + game.halite_on_position(closest_ship->position) < constants::DROPOFF_COST);
				game.me->dropoffs[objective_id--] = make_shared<Dropoff>(Dropoff(game.my_id, -1, dropoff_objective.target_position.x, dropoff_objective.target_position.y, is_fake));
				log::log(closest_ship->to_string_ship() + " is close to target dropoff, placed dropoff");
			}

			log::log(closest_ship->to_string_ship() + " assigned to dropoff " + dropoff_objective.target_position.to_string_position());
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
			game.assign_objective(ship, Objective_Type::SUICIDE_ON_BASE, game.get_closest_shipyard_or_dropoff(ship));
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
			game.assign_objective(ship, Objective_Type::BACK_TO_BASE, game.get_closest_shipyard_or_dropoff(ship, false));
			ship->set_assigned();
		}
	}

	/*
	Update EXTRACT_ZONE
	**********************
	In globally greedy assignment
	*/
	{
		Stopwatch s("Extract zones");
		unordered_map<shared_ptr<Ship>, double> ships_without_objectives;
		for (const shared_ptr<Ship>& ship : game.me->my_ships)
			if (!ship->assigned)
				ships_without_objectives[ship] = 0.0;

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

			game.scorer.decreases_score_in_target_cell(best_ship, best_cell, 0.0, game);
			game.scorer.decreases_score_in_target_area(best_ship, best_cell, game.get_constant("Score: Brute force reach"), game);
			game.assign_objective(best_ship, Objective_Type::EXTRACT_ZONE, best_cell->position, best_score);
			ships_without_objectives.erase(best_ship);
		}
	}
}

void ObjectiveManager::get_ordered_ships(Game& game)
{
	PriorityQueue<shared_ptr<Ship>, double> ships_dropoff;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::MAKE_DROPOFF))
			ships_dropoff.put(ship, ship->objective->score);

	while (!ships_dropoff.empty())
		ships_ordered.push_back(ships_dropoff.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_suicide;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
			ships_suicide.put(ship, ship->objective->score);

	while (!ships_suicide.empty())
		ships_ordered.push_back(ships_suicide.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_back_to_base;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::BACK_TO_BASE))
			ships_back_to_base.put(ship, ship->objective->score);

	while (!ships_back_to_base.empty())
		ships_ordered.push_back(ships_back_to_base.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_extract;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::EXTRACT_ZONE))
			ships_extract.put(ship, -ship->objective->score);

	while (!ships_extract.empty())
		ships_ordered.push_back(ships_extract.get());

	if (ships_ordered.size() != game.me->ships.size())
	{
		log::log("Not all ships selected");
		exit(1);
	}

	//log::log("Queue:");
	//for (auto& s : ships_ordered)
	//	log::log(s->to_string_ship());
}

void ObjectiveManager::flush_objectives()
{
	ships_ordered.clear();
}