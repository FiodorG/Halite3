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
		max_dropoffs = (game.players.size() == 2) ? 3 : 1;
		break;
	case 40:
		max_dropoffs = (game.players.size() == 2) ? 3 : 2;
		break;
	case 48:
		max_dropoffs = (game.players.size() == 2) ? 4 : 3;
		break;
	case 56:
		max_dropoffs = (game.players.size() == 2) ? 5 : 4;
		break;
	case 64:
		max_dropoffs = (game.players.size() == 2) ? 8 : 7;
		break;
	default:
		log::log("Unknown map width");
		exit(1);
	}

	int base_dropoffs;

	if (game.is_four_player_game())
	{
		if (game.scorer.halite_initial <= 150000)
			base_dropoffs = 0;
		else if (game.scorer.halite_initial <= 250000)
			base_dropoffs = 1;
		else if (game.scorer.halite_initial <= 400000)
			base_dropoffs = 2;
		else if (game.scorer.halite_initial <= 600000)
			base_dropoffs = 3;
		else if (game.scorer.halite_initial <= 800000)
			base_dropoffs = 4;
		else if (game.scorer.halite_initial <= 1000000)
			base_dropoffs = 5;
		else if (game.scorer.halite_initial <= 1200000)
			base_dropoffs = 6;
		else
			base_dropoffs = 7;
	}
	else
	{
		if (game.scorer.halite_initial <= 120000)
			base_dropoffs = 0;
		else if (game.scorer.halite_initial <= 240000)
			base_dropoffs = 1;
		else if (game.scorer.halite_initial <= 360000)
			base_dropoffs = 2;
		else if (game.scorer.halite_initial <= 480000)
			base_dropoffs = 3;
		else if (game.scorer.halite_initial <= 600000)
			base_dropoffs = 4;
		else if (game.scorer.halite_initial <= 720000)
			base_dropoffs = 5;
		else if (game.scorer.halite_initial <= 840000)
			base_dropoffs = 6;
		else if (game.scorer.halite_initial <= 960000)
			base_dropoffs = 7;
		else
			base_dropoffs = 8;
	}

	base_dropoffs = min(base_dropoffs, max_dropoffs);

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

	if (game.turns_remaining_percent() <= 0.33)
		return false;

	if (game.my_dropoff_number() == 0)
		return (game.my_ships_number() >= (game.is_four_player_game() && (game.game_map->width <= 40)) ? 13 : 15);
	else if (game.my_dropoff_number() == 1)
		return (game.my_ships_number() >= 30);
	else if (game.my_dropoff_number() == 2)
		return (game.my_ships_number() >= 45) && (turn_since_last_dropoff >= 40);
	else if (game.my_dropoff_number() == 3)
		return (game.my_ships_number() >= 60) && (turn_since_last_dropoff >= 40);
	else if (game.my_dropoff_number() == 4)
		return (game.my_ships_number() >= 80) && (turn_since_last_dropoff >= 40);
	else if (game.my_dropoff_number() == 5)
		return (turn_since_last_dropoff >= 40);
	else if (game.my_dropoff_number() == 6)
		return (turn_since_last_dropoff >= 40);
	else if (game.my_dropoff_number() == 7)
		return (turn_since_last_dropoff >= 40);

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
				game.reserved_halite += max(constants::DROPOFF_COST - closest_ship->halite - game.halite_on_position(closest_ship->position), 0);
				log::log(closest_ship->to_string_ship() + " is close to target dropoff, reserved halite " + to_string(game.reserved_halite));
			}

			if ((game.distance_from_objective(closest_ship) <= 5) && (game.me->halite + closest_ship->halite + game.halite_on_position(closest_ship->position) >= constants::DROPOFF_COST))
			{
				game.me->dropoffs[objective_id--] = make_shared<Dropoff>(Dropoff(game.my_id, -1, dropoff_objective.target_position.x, dropoff_objective.target_position.y, false));
				log::log(closest_ship->to_string_ship() + " is close to target dropoff, placed fake dropoff");
			}

			log::log(closest_ship->to_string_ship() + " assigned to dropoff " + dropoff_objective.target_position.to_string_position());
		}

		// refresh shipyards to account for potentially newly placed
		game.distance_manager.fill_closest_shipyard_or_dropoff(game);
	}

	/*
	Update BLOCK_ENEMY_BASE
	**********************
	*/
	{
		Stopwatch s("Greedy allocation Block");
		unordered_map<shared_ptr<Ship>, double> ships_to_block;
		for (const shared_ptr<Ship>& ship : game.me->my_ships)
			if (
				!ship->assigned &&
				(
					// if previously assigned to blockade
					(ship->is_objective(Objective_Type::BLOCK_ENEMY_BASE) && (ship->halite < 400)) ||
					// capture ships to blockade
					((ship->halite < 40) && (2 * game.distance(ship->position, game.get_closest_enemy_shipyard_or_dropoff(ship)) >= game.turns_remaining())) ||
					// also do when no halite left
					(game.get_constant("Test") && (ship->halite < 40) && (game.scorer.halite_total < 5000)) ||
					// convert ships to blockade from suicide if low halite
					(ship->is_objective(Objective_Type::SUICIDE_ON_BASE) && (ship->halite < 40))
				)
			)
				ships_to_block[ship] = 0.0;

		while (ships_to_block.size())
		{
			shared_ptr<Ship> best_ship;
			Objective best_objective = Objective();
			double best_score = -DBL_MAX;

			for (const auto& ship : ships_to_block)
			{
				Objective objective = game.blocker.find_best_objective_cell(ship.first, game);

				if (objective.score > best_score)
				{
					best_ship = ship.first;
					best_score = objective.score;
					best_objective = objective;
				}
			}

			log::log(best_ship->to_string_ship() + " blocking area " + best_objective.target_position.to_string_position() + " with score " + to_string(best_score));

			game.blocker.decrease_score_in_position(best_objective.target_position, game);
			best_ship->assign_objective(best_objective);
			best_ship->set_assigned();
			ships_to_block.erase(best_ship);
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

		if (
			ship->is_objective(Objective_Type::SUICIDE_ON_BASE) || 
			(2 * game.distance(ship->position, game.get_closest_shipyard_or_dropoff(ship, false)) >= game.turns_remaining()) ||
			(game.turns_remaining() <= 6)
		)
		{
			game.assign_objective(ship, Objective_Type::SUICIDE_ON_BASE, game.get_closest_shipyard_or_dropoff(ship, false));
			ship->set_assigned();
		}
	}

	/*
	Update RETURN_TO_BASE
	**********************
	*/
	double cargo_fullness;

	for (const shared_ptr<Ship>& ship : game.me->my_ships)
	{
		if (ship->assigned)
			continue;

		if (game.switch_to_half_full_for_rtb(ship))
			cargo_fullness = 0.5;
		else
			cargo_fullness = 0.9;

		if (ship->is_objective(Objective_Type::BACK_TO_BASE) && game.is_shipyard_or_dropoff(ship->position))
			ship->clear_objective();

		if ((ship->is_objective(Objective_Type::BACK_TO_BASE)) || (ship->is_full(cargo_fullness)))
		{
			game.assign_objective(ship, Objective_Type::BACK_TO_BASE, game.distance_manager.get_closest_shipyard_or_dropoff(ship));
			ship->set_assigned();
		}
	}

	/*
	Update EXTRACT_ZONE
	**********************
	In globally greedy assignment
	*/
	{
		Stopwatch s("Greedy allocation");
		unordered_map<shared_ptr<Ship>, double> ships_without_objectives;
		for (const shared_ptr<Ship>& ship : game.me->my_ships)
			if (!ship->assigned)
				ships_without_objectives[ship] = 0.0;

		while (ships_without_objectives.size())
		{
			shared_ptr<Ship> best_ship;
			Objective best_objective = Objective();
			double best_score = -DBL_MAX;

			for (const auto& ship : ships_without_objectives)
			{
				Objective objective = game.scorer.find_best_objective_cell(ship.first, game);

				if (objective.score > best_score)
				{
					best_ship = ship.first;
					best_score = objective.score;
					best_objective = objective;
				}
			}

			if (best_objective.type == Objective_Type::ATTACK)
			{
				log::log(best_ship->to_string_ship() + " attacking enemy on " + best_objective.target_position.to_string_position() + " with score " + to_string(best_score));

				game.ship_on_position(best_objective.target_position)->set_targeted();
				best_ship->assign_objective(best_objective);
			}
			else if (best_objective.type == Objective_Type::EXTRACT_ZONE)
			{
				log::log(best_ship->to_string_ship() + " assigned to area " + best_objective.target_position.to_string_position() + " with score " + to_string(best_score));

				game.scorer.decreases_score_in_target_cell(best_ship, best_objective.target_position, 0.0, game);
				game.scorer.decreases_score_in_target_area(best_ship, best_objective.target_position, game);
				game.assign_objective(best_ship, Objective_Type::EXTRACT_ZONE, best_objective.target_position, best_score);
			}
			else
			{
				log::log("Unknown objective in greedy method");
				exit(1);
			}

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

	PriorityQueue<shared_ptr<Ship>, double> ships_block;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::BLOCK_ENEMY_BASE))
			ships_block.put(ship, -ship->objective->score);

	while (!ships_block.empty())
		ships_ordered.push_back(ships_block.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_suicide;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
			ships_suicide.put(ship, -ship->halite);

	while (!ships_suicide.empty())
		ships_ordered.push_back(ships_suicide.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_back_to_base;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::BACK_TO_BASE))
			ships_back_to_base.put(ship, ship->objective->score);

	while (!ships_back_to_base.empty())
		ships_ordered.push_back(ships_back_to_base.get());

	PriorityQueue<shared_ptr<Ship>, double> ships_attack;
	for (const shared_ptr<Ship>& ship : game.me->my_ships)
		if (ship->is_objective(Objective_Type::ATTACK))
			ships_attack.put(ship, -ship->objective->score);

	while (!ships_attack.empty())
		ships_ordered.push_back(ships_attack.get());

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