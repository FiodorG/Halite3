#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include "constants.hpp"

#include <vector>

using namespace std;

namespace hlt 
{
	struct Game;
    struct GameMap 
	{
        int width;
        int height;
        vector<vector<MapCell>> cells;

        MapCell* at(const Position& position) 
		{
            Position normalized = normalize(position);
            return &cells[normalized.y][normalized.x];
        }

        MapCell* at(const Entity& entity) 
		{
            return at(entity.position);
        }

        MapCell* at(const Entity* entity) 
		{
            return at(entity->position);
        }

        MapCell* at(const shared_ptr<Entity>& entity) 
		{
            return at(entity->position);
        }

		Position normalize(const Position& position)
		{
			const int x = ((position.x % width) + width) % width;
			const int y = ((position.y % height) + height) % height;
			return { x, y };
		}

		int calculate_distance(const Position& source, const Position& target)
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_target = normalize(target);

			const int dx = abs(normalized_source.x - normalized_target.x);
			const int dy = abs(normalized_source.y - normalized_target.y);

			const int toroidal_dx = min(dx, width - dx);
			const int toroidal_dy = min(dy, height - dy);

			return toroidal_dx + toroidal_dy;
		}

		vector<Direction> get_unsafe_moves(const Position& source, const Position& destination)
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_destination = normalize(destination);

			const int dx = abs(normalized_source.x - normalized_destination.x);
			const int dy = abs(normalized_source.y - normalized_destination.y);
			const int wrapped_dx = width - dx;
			const int wrapped_dy = height - dy;

			vector<Direction> possible_moves;

			if ((normalized_source.x == normalized_destination.x) && (normalized_source.y == normalized_destination.y))
			{
				possible_moves.push_back(Direction::STILL);
				return possible_moves;
			}

			if (normalized_source.x < normalized_destination.x)
				possible_moves.push_back(dx > wrapped_dx ? Direction::WEST : Direction::EAST);
			else if (normalized_source.x > normalized_destination.x)
				possible_moves.push_back(dx < wrapped_dx ? Direction::WEST : Direction::EAST);

			if (normalized_source.y < normalized_destination.y)
				possible_moves.push_back(dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
			else if (normalized_source.y > normalized_destination.y)
				possible_moves.push_back(dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);

			return possible_moves;
		}

		vector<Direction> navigate(shared_ptr<Ship> ship, const Position& destination)
		{
			// Move resource checking here
			return get_unsafe_moves(ship->position, destination);
		}

		Direction naive_navigate(shared_ptr<Ship> ship, const Position& destination)
		{
			for (auto direction : get_unsafe_moves(ship->position, destination))
			{
				Position target_pos = ship->position.directional_offset(direction);
				if (!at(target_pos)->is_occupied())
				{
					at(target_pos)->mark_unsafe(ship);
					return direction;
				}
			}

			return Direction::STILL;
		}

		double scoring_function(const MapCell& source_cell, const MapCell& target_cell, const Game& game);
		MapCell* closest_cell_with_ressource(const Entity& entity, const Game& game);

		/* Next turn functions */
        void _update();
        static unique_ptr<GameMap> _generate();
    };
}

