#pragma once

#include "entity.hpp"

#include <memory>

namespace hlt 
{
    struct Dropoff : Entity 
	{
		bool fake;

		Dropoff(PlayerId owner, EntityId id, int x, int y) : Entity(owner, id, x, y), fake(false) {}
		Dropoff(PlayerId owner, EntityId id, int x, int y, bool fake) : Entity(owner, id, x, y), fake(fake) {}
        static std::shared_ptr<Dropoff> _generate(PlayerId player_id);

		string to_string_dropoff() const { return "Dropoff(" + position.to_string_position() + ")"; }
    };
}
