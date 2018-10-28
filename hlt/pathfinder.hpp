#pragma once

#include "position.hpp"
#include "direction.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "command.hpp"
#include "map_cell.hpp"

#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_map>

using namespace std;

namespace hlt
{
	struct Game;
	class PathFinder
	{
		private:
		int width;
		
		public:
		PathFinder() : width(0) {}
		PathFinder(int width) : width(width) {}

		// Dijkstra
		struct PriorityQueue 
		{
			typedef pair<int, MapCell*> PQElement;
			priority_queue<PQElement, vector<PQElement>, greater<PQElement>> elements;

			inline void put(MapCell* mapcell, int priority) { elements.emplace(priority, mapcell); }
			inline bool empty() const { return elements.empty(); }

			MapCell* get()
			{
				MapCell* best_item = elements.top().second;
				elements.pop();
				return best_item;
			}
		};

		Position compute_shortest_path(const Position& source_position, const Position& target_position, const Game& game) const;
		private:
		static vector<MapCell*> reconstruct_path(MapCell* source_cell, MapCell* target_cell, unordered_map<MapCell*, MapCell*> came_from);
		static vector<MapCell*> adjacent_cells(MapCell* source_cell, MapCell* target_cell, MapCell* cell, const Game& game);
		inline int heuristic(MapCell* cell, MapCell* target_cell, const Game& game) const;
		int compute_next_step_score(MapCell* source_cell, MapCell* next_cell, const Game& game) const;
		vector<MapCell*> dijkstra(MapCell* source_cell, MapCell* target_cell, const Game& game) const;
	};
}