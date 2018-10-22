#pragma once

#include "types.hpp"
#include "direction.hpp"

#include <iostream>
#include <string>

using namespace std;

namespace hlt 
{
    struct Position 
	{
        int x;
        int y;

		Position() : x(0), y(0) {}
        Position(int x, int y) : x(x), y(y)  {}
		Position(const Position& position) : x(position.x), y(position.y) {}

        bool operator==(const Position& other) const { return x == other.x && y == other.y; }
        bool operator!=(const Position& other) const { return x != other.x || y != other.y; }

		static Direction perpendicular_direction_left(Direction d)
		{
			switch (d)
			{
			case Direction::NORTH:
				return Direction::WEST;
			case Direction::SOUTH:
				return Direction::EAST;
			case Direction::EAST:
				return Direction::SOUTH;
			case Direction::WEST:
				return Direction::NORTH;
			case Direction::STILL:
				return Direction::STILL;
			default:
				log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(d));
				exit(1);
			}
		}

		static Direction perpendicular_direction_right(Direction d)
		{
			switch (d)
			{
			case Direction::NORTH:
				return Direction::EAST;
			case Direction::SOUTH:
				return Direction::WEST;
			case Direction::EAST:
				return Direction::NORTH;
			case Direction::WEST:
				return Direction::SOUTH;
			case Direction::STILL:
				return Direction::STILL;
			default:
				log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(d));
				exit(1);
			}
		}

        Position directional_offset(Direction d) const 
		{
            auto dx = 0;
            auto dy = 0;
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
                    // No move
                    break;
                default:
                    log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(d));
                    exit(1);
            }
            return Position{x + dx, y + dy};
        }

        array<Position, 4> get_surrounding_cardinals() 
		{
            return {{
                directional_offset(Direction::NORTH), directional_offset(Direction::SOUTH),
                directional_offset(Direction::EAST), directional_offset(Direction::WEST)
            }};
        }

		string to_string_position()
		{
			return "(" + to_string(x) + "," + to_string(y) + ")";
		}
    };

    static ostream& operator<<(ostream& out, const Position& position) 
	{
        out << position.x << ' ' << position.y;
        return out;
    }

    static istream& operator>>(istream& in, Position& position) 
	{
        in >> position.x >> position.y;
        return in;
    }
}

namespace std 
{
    template <> struct hash<hlt::Position> 
	{
        size_t operator()(const hlt::Position& position) const 
		{
            return ((position.x+position.y) * (position.x+position.y+1) / 2) + position.y;
        }
    };
}

