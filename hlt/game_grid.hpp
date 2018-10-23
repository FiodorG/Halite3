#pragma once

#include "position.hpp"
#include "direction.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "command.hpp"
#include "map_cell.hpp"

#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

namespace hlt
{
	struct Game;
	class GameGrid
	{
		private:
		int width;
		
		public:
		GameGrid() : width(0) {}
		GameGrid(int width) : width(width) {}

		int to_index(MapCell* cell) const { return cell->position.x * width + cell->position.y; }

		// Dijkstra
		public:
		Position compute_shortest_path(const Position& source_position, const Position& target_position, const Game& game) const;
		private:
		static MapCell* get_lowest_distance_cell(const list<MapCell*>& unsettled_cells, const list<MapCell*>& settled_cells, const Game& game);
		static vector<MapCell*> adjacent_cells(MapCell* cell, const Game& game);
		list<Position> dijkstra(MapCell* source_cell, MapCell* target_cell, const Game& game) const;
	};
}