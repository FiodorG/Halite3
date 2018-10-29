#pragma once

#include "types.hpp"
#include "position.hpp"
#include "ship.hpp"
#include "dropoff.hpp"

using namespace std;

namespace hlt 
{
    struct MapCell
	{
        Position position;
        Halite halite;
        shared_ptr<Ship> ship;
        shared_ptr<Entity> structure;

        MapCell(int x, int y, Halite halite) :
            position(x, y),
            halite(halite)
        {}

        bool is_empty() const { return !ship && !structure; }
        bool is_occupied() const { return static_cast<bool>(ship); }
		bool is_occupied_by_ally(PlayerId id) const { return static_cast<bool>(ship) && (ship->owner == id); }
		bool is_occupied_by_enemy(PlayerId id) const { return static_cast<bool>(ship) && (ship->owner != id); }
        bool has_structure() const { return static_cast<bool>(structure); }
		bool has_base(int playerid) const { return static_cast<bool>(structure) && structure->owner == playerid; }
        void mark_unsafe(shared_ptr<Ship>& ship) { this->ship = ship; }
    };
}
