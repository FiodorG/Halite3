#include "objective_manager.hpp"
#include "game.hpp"

using namespace hlt;
using namespace std;

void ObjectiveManager::create_dropoff_objective(const Game& game)
{
	if (!should_spawn_dropoff(game))
		return;

	vector<Position> dropoffs;
	dropoffs.push_back(game.my_shipyard_position());
	for (auto& dropoff : game.me->dropoffs)
		dropoffs.push_back(dropoff.second->position);

	for (int i = game.my_dropoff_number(); i < game.max_allowed_dropoffs(); i++)
	{
		pair<MapCell*, double> action = game.scorer.find_best_dropoff_cell(game.me->shipyard, dropoffs, game);
		dropoffs.push_back(action.first->position);
		objectives_dropoffs.push_back(Objective(0, Objective_Type::MAKE_DROPOFF, action.first->position));
		log::log("Dropoff objective: " + action.first->position.to_string_position() + " with score " + to_string(action.second));
		break;
	}
}

bool ObjectiveManager::should_spawn_dropoff(const Game& game)
{
	if (
		(game.my_ships_number() >= 20) &&
		(turn_since_last_dropoff >= 100)
	)
	{
		return true;
	}

	return false;
}

bool ObjectiveManager::can_spawn_dropoff(const shared_ptr<Ship> ship, Game& game)
{
	if (
		ship->is_objective(Objective_Type::MAKE_DROPOFF) &&
		(game.distance_from_objective(ship) <= 1)  &&
		(game.me->halite + ship->halite + game.mapcell(ship)->halite >= 5 + constants::DROPOFF_COST)
		)
	{
		return true;
		game.objective_manager.turn_since_last_dropoff = 0;
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

		create_dropoff_objective(game);
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

			if (game.distance_from_objective(closest_ship) <= 5)
			{
				game.me->dropoffs[objective_id--] = make_shared<Dropoff>(Dropoff(game.my_id, -1, dropoff_objective.target_position.x, dropoff_objective.target_position.y));
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
			game.assign_objective(ship, Objective_Type::BACK_TO_BASE, game.get_closest_shipyard_or_dropoff(ship));
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

			if (game.get_constant("Test"))
				game.scorer.decreases_score_in_target_cell(best_ship, best_cell, game);

			game.scorer.decreases_score_in_target_area(best_ship, best_cell, game.get_constant("Score: Brute force reach"), game);
			game.assign_objective(best_ship, Objective_Type::EXTRACT_ZONE, best_cell->position, best_score);
			ships_without_objectives.erase(best_ship);
		}
	}
}

void ObjectiveManager::get_ordered_ships(Game& game)
{
	Stopwatch s("Order ships");

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
	objectives_dropoffs.clear();
}