#pragma once

#include "types.hpp"
#include "shipyard.hpp"
#include "ship.hpp"
#include "dropoff.hpp"
#include "input.hpp"

#include <memory>
#include <unordered_map>
#include <queue>

using namespace std;

namespace hlt 
{
    struct Player 
	{
        PlayerId id;
        shared_ptr<Shipyard> shipyard;
        Halite halite;
        unordered_map<EntityId, shared_ptr<Ship>> ships;
        unordered_map<EntityId, shared_ptr<Dropoff>> dropoffs;
		list<shared_ptr<Ship>> ships_ordered;

        Player(PlayerId player_id, int shipyard_x, int shipyard_y) :
            id(player_id),
            shipyard(make_shared<Shipyard>(player_id, shipyard_x, shipyard_y)),
            halite(0)
        {}

		// Functions for new turn logic
		void _update(int num_ships, int num_dropoffs, Halite halite);
		static shared_ptr<Player> _generate();
    };
}
