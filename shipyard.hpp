#pragma once

#include "entity.hpp"
#include "ship.hpp"
#include "command.hpp"

#include <list>

using namespace std;

namespace hlt 
{
    struct Shipyard : Entity 
	{
		Shipyard(PlayerId owner, int x, int y) : Entity(owner, -1, x, y) {}

        Command spawn() { return hlt::command::spawn_ship(); }
    };
}
