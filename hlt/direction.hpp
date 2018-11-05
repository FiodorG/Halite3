#pragma once

#include "log.hpp"

#include <ostream>
#include <array>

using namespace std;

namespace hlt 
{
    enum class Direction : char 
	{
        NORTH = 'n',
        EAST = 'e',
        SOUTH = 's',
        WEST = 'w',
        STILL = 'o',
    };

    static const array<Direction, 5> ALL_CARDINALS = 
	{
        { Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST, Direction::STILL }
    };

    static Direction invert_direction(Direction direction) 
	{
        switch (direction) 
		{
            case Direction::NORTH:
                return Direction::SOUTH;
            case Direction::SOUTH:
                return Direction::NORTH;
            case Direction::EAST:
                return Direction::WEST;
            case Direction::WEST:
                return Direction::EAST;
            case Direction::STILL:
                return Direction::STILL;
            default:
                log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(direction));
                exit(1);
        }
    }

	static Direction get_perpendicular_direction_left(Direction direction)
	{
		switch (direction)
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
				log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(direction));
				exit(1);
		}
	}

	static Direction get_perpendicular_direction_right(Direction direction)
	{
		switch (direction)
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
				log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(direction));
				exit(1);
		}
	}

	static string to_string_direction(const Direction& direction)
	{
		switch (direction)
		{
		case Direction::NORTH:
			return "n";
		case Direction::SOUTH:
			return "s";
		case Direction::EAST:
			return "e";
		case Direction::WEST:
			return "w";
		case Direction::STILL:
			return "o";
		default:
			log::log(string("Error: invert_direction: unknown direction ") + static_cast<char>(direction));
			exit(1);
		}
	}

    static ostream& operator<<(ostream& out, const Direction& direction) 
	{
        return out << static_cast<char>(direction);
    }
}
