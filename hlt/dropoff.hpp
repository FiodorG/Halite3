#pragma once

#include "entity.hpp"

#include <memory>

namespace hlt 
{
    struct Dropoff : Entity 
	{
        using Entity::Entity;
        static std::shared_ptr<Dropoff> _generate(PlayerId player_id);

		string to_string_dropoff() const { return "Dropoff(" + position.to_string_position() + ")"; }
    };
}
