#pragma once

#include "position.hpp"
#include "direction.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "command.hpp"
#include "map_cell.hpp"
#include "priority_queue.hpp"

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

		Position compute_path(shared_ptr<Ship> ship, const Position& target_position, Game& game);
		Position compute_direct_path(const Position& source_position, const Position& target_position, Game& game);
		Position compute_direct_path_no_base(const Position& source_position, const Position& target_position, Game& game);
		Position compute_direct_path_suicide(const Position& source_position, const Position& target_position, Game& game);
		Position compute_direct_path_rtb(const Position& source_position, const Position& target_position, Game& game);
		Position compute_direct_path_attack(const Position& source_position, const Position& target_position, Game& game);

		// Dijkstra suicide
		vector<MapCell*> dijkstra_suicide(MapCell* source_cell, MapCell* target_cell, MapCell* enemy_base, const Game& game) const;
		int compute_next_step_score_suicide(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, MapCell* enemy_base, const Game& game) const;

		// Dijkstra block
		vector<MapCell*> dijkstra_block(MapCell* source_cell, MapCell* target_cell, MapCell* enemy_base, const Game& game) const;
		int compute_next_step_score_block(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, MapCell* enemy_base, const Game& game) const;

		// Dijkstra RTB
		vector<MapCell*> dijkstra_rtb(MapCell* source_cell, MapCell* target_cell, const Game& game) const;
		int compute_next_step_score_rtb(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, const Game& game) const;

		// Dijkstra Attack
		vector<MapCell*> dijkstra_attack(MapCell* source_cell, MapCell* target_cell, const Game& game) const;
		int compute_next_step_score_attack(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, const Game& game) const;

		// Dijkstra
		Position compute_shortest_path(const Position& source_position, const Position& target_position, Game& game);
		static vector<MapCell*> reconstruct_path(MapCell* source_cell, MapCell* target_cell, unordered_map<MapCell*, MapCell*> came_from);
		static vector<MapCell*> adjacent_cells_filtered(MapCell* source_cell, MapCell* target_cell, MapCell* cell, const Game& game);
		static vector<MapCell*> adjacent_cells_all(MapCell* cell, const Game& game);
		inline int heuristic(MapCell* cell, MapCell* target_cell, const Game& game) const;
		int compute_next_step_score(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, const Game& game) const;
		vector<MapCell*> dijkstra_path(MapCell* source_cell, MapCell* target_cell, const Game& game) const;

		void log_path(vector<MapCell*>& optimal_path, const Game& game) const;
		void log_costs(unordered_map<MapCell*, int> cost_so_far, const Game& game) const;
	};
}