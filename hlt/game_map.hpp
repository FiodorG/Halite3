#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include "constants.hpp"

#include <vector>
#include <math.h>

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
		MapCell* at(int x, int y) { return at(normalize(Position(x, y))); }
		MapCell* at(const Entity& entity) { return at(entity.position); }
		MapCell* at(const Entity* entity) { return at(entity->position); }
		MapCell* at(const shared_ptr<Entity>& entity) { return at(entity->position); }

		Position directional_offset(const Position& position, Direction d) const
		{
			int dx = 0;
			int dy = 0;
			switch (d)
			{
			case Direction::NORTH:
				dy = -1;
				break;
			case Direction::SOUTH:
				dy = 1;
				break;
			case Direction::EAST:
				dx = 1;
				break;
			case Direction::WEST:
				dx = -1;
				break;
			case Direction::STILL:
				break;
			default:
				log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(d));
				exit(1);
			}

			int new_x = position.x + dx;
			int new_y = position.y + dy;

			return Position(((new_x % width) + width) % width, ((new_y % height) + height) % height);
		}

		Position normalize(const Position& position) const
		{
			const int x = ((position.x % width) + width) % width;
			const int y = ((position.y % height) + height) % height;
			return { x, y };
		}

		inline int calculate_distance(const Position& source, const Position& target) const
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_target = normalize(target);

			const int dx = abs(normalized_source.x - normalized_target.x);
			const int dy = abs(normalized_source.y - normalized_target.y);

			return min(dx, width - dx) + min(dy, height - dy);
		}

		int calculate_distance_inf(const Position& source, const Position& target) const
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_target = normalize(target);

			const int dx = abs(normalized_source.x - normalized_target.x);
			const int dy = abs(normalized_source.y - normalized_target.y);

			const int toroidal_dx = min(dx, width - dx);
			const int toroidal_dy = min(dy, height - dy);

			return max(toroidal_dx, toroidal_dy);
		}

		int calculate_distance_from_axis(const Position& source, const Position& target) const
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_target = normalize(target);

			const int dx = abs(normalized_source.x - normalized_target.x);
			const int dy = abs(normalized_source.y - normalized_target.y);

			const int toroidal_dx = min(dx, width - dx);
			const int toroidal_dy = min(dy, height - dy);

			return min(toroidal_dx, toroidal_dy);
		}

		Direction get_move(const Position& source, const Position& destination)
		{
			const auto& normalized_source = normalize(source);
			const auto& normalized_destination = normalize(destination);

			const int dx = abs(normalized_source.x - normalized_destination.x);
			const int dy = abs(normalized_source.y - normalized_destination.y);
			const int wrapped_dx = width - dx;
			const int wrapped_dy = height - dy;

			if (normalized_source.x < normalized_destination.x)
				return (dx > wrapped_dx ? Direction::WEST : Direction::EAST);
			else if (normalized_source.x > normalized_destination.x)
				return (dx < wrapped_dx ? Direction::WEST : Direction::EAST);
			else if (normalized_source.y < normalized_destination.y)
				return (dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
			else if (normalized_source.y > normalized_destination.y)
				return (dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);

			return Direction::STILL;
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

		/* Next turn functions */
        void _update();
        static unique_ptr<GameMap> _generate();
    };
}

