#pragma once

#include "entity.hpp"
#include "ship.hpp"
#include "command.hpp"

#include <list>

namespace hlt 
{
    struct Shipyard : Entity 
	{
		std::list<std::shared_ptr<Ship>> assigned_ships;
		int n_assigned_ships;

        Shipyard(PlayerId owner, int x, int y) : 
			Entity(owner, -1, x, y),
			n_assigned_ships(0)
		{
		}

		void reset_assigned_ships()
		{
			assigned_ships.clear();
			n_assigned_ships = 0;
		}

		void assign_ship(std::shared_ptr<Ship> ship)
		{
			assigned_ships.push_back(ship);
			n_assigned_ships++;
		}

        Command spawn() 
		{
            return hlt::command::spawn_ship();
        }
    };
}
