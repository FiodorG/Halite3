#pragma once

#include "entity.hpp"
#include "constants.hpp"
#include "command.hpp"
#include "objective.hpp"

#include <memory>

namespace hlt 
{
    struct Ship : Entity 
	{
        Halite halite;
		shared_ptr<Objective> objective;
		bool assigned;

        Ship(PlayerId player_id, EntityId ship_id, int x, int y, Halite halite) :
            Entity(player_id, ship_id, x, y),
            halite(halite),
			assigned(false)
        {}

		void set_assigned() { this->assigned = true; }
        bool is_full(double percentage = 1.0) const { return halite >= percentage * constants::MAX_HALITE; }
		void assign_objective(shared_ptr<Objective> objective) { this->objective = objective; }
		void clear_objective() { this->objective.reset(); }
		bool has_objective() const { return static_cast<bool>(objective); }

		void assign_objective(shared_ptr<Ship> ship)
		{
			if (static_cast<bool>(ship->objective))
				this->objective = ship->objective;
		}

		bool is_objective(Objective_Type type) const
		{
			if (has_objective())
				return (objective->type == type);
			else
				return false;
		}
		
		Position target_position()
		{
			if (has_objective())
				return this->objective->target_position;
			else
				throw runtime_error("Ship" + to_string(id) + " has no objective");
		}

		bool is_at_objective() const
		{
			if (has_objective())
				return (this->position == this->objective->target_position);
			else
				return false;
		}


		/* Logging & Internal */
		string to_string_ship()
		{
			string assigned = (this->assigned ? "*" : "");

			if (static_cast<bool>(objective))
				return "Ship" + assigned + "(" + to_string(id) + "," + to_string(halite) + "," + position.to_string_position() + "," + objective->to_string_objective() + ")";
			else
				return "Ship" + assigned + "(" + to_string(id) + "," + to_string(halite) + "," + position.to_string_position() + ")";
		}

		bool operator==(const Ship& other) const { return owner == other.owner && id == other.id; }
		bool operator!=(const Ship& other) const { return owner != other.owner || id != other.id; }


		/* Commands */
        Command make_dropoff() const { return hlt::command::transform_ship_into_dropoff_site(id); }
        Command move(Direction direction) const { return hlt::command::move(id, direction); }
        Command stay_still() const { return hlt::command::move(id, Direction::STILL); }


		/* New turn */
		static std::shared_ptr<Ship> _generate(PlayerId player_id);
    };
}

