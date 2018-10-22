#pragma once

#include "position.hpp"
#include "direction.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "command.hpp"

#include <vector>
#include <unordered_map>

using namespace std;

namespace hlt
{
	struct Game;
	struct NavigationManager
	{
		unordered_map<shared_ptr<Ship>, Position> positions_next_turn;
		unordered_map<shared_ptr<Ship>, Position> positions_enemies;

		NavigationManager() {}

		bool shipyard_occupied_next_turn(const Shipyard& shipyard)
		{
			for (auto& ship_position : positions_next_turn)
				if (ship_position.second == shipyard.position)
					return true;

			return false;
		}

		unordered_map<shared_ptr<Ship>, Position> find_any_collisions(const Game& game);
		unordered_map<shared_ptr<Ship>, Position> find_any_enemy_collisions(const Game& game);
		bool position_collides_with_existing(shared_ptr<Ship> ship, const Position& position);
		void edit_collisions(unordered_map<shared_ptr<Ship>, Position> collisions, unordered_map<shared_ptr<Ship>, vector<Direction>> moves_queue, const Game& game);
		void fill_positions_next_turn(const unordered_map<shared_ptr<Ship>, vector<Direction>>& moves_queue, const Game& game);
		void fill_positions_enemies(const Game& game);
		vector<Command> resolve_moves(const unordered_map<shared_ptr<Ship>, vector<Direction>>& moves_queue, const Game& game);
	};
}