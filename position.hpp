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

		string to_string_position() const { return "(" + to_string(x) + "," + to_string(y) + ")"; }
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
            return ((position.x + position.y) * (position.x+position.y + 1) / 2) + position.y;
        }
    };
}

