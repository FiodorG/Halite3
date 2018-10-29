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
	class CollisionResolver
	{
		private:
		unordered_map<shared_ptr<Ship>, Position> positions_enemies;

		public:
		CollisionResolver() {}

		// Move Resolve
		unordered_map<shared_ptr<Ship>, Position> find_any_collisions(const Game& game);
		unordered_map<shared_ptr<Ship>, Position> find_any_enemy_collisions(const Game& game);
		bool position_collides_with_existing(shared_ptr<Ship> ship, const Position& position, const Game& game);
		bool is_ship_switching_places(shared_ptr<Ship> ship, Game& game) const;
		void edit_collisions(unordered_map<shared_ptr<Ship>, Position> collisions, Game& game);
		void fill_positions_enemies(Game& game);
		void exchange_ships(Game& game);
		vector<Command> resolve_moves(Game& game);
	};
}