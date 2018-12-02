#include "pathfinder.hpp"
#include "game.hpp"

#include <cmath>
#include <limits.h>

using namespace hlt;
using namespace std;

void PathFinder::log_path(vector<MapCell*>& optimal_path, const Game& game) const
{
	log::log("Optimal path:");
	for (int y = 0; y < game.game_map->height; ++y)
	{
		string padding = (y <= 9) ? "0" : "";
		string line = "" + padding + to_string(y) + " | ";

		for (int x = 0; x < game.game_map->width; ++x)
			if (find(optimal_path.begin(), optimal_path.end(), &game.game_map->cells[y][x]) != optimal_path.end())
				line += "X ";
			else
				line += "O ";

		log::log(line);
	}
	log::log("");
}
void PathFinder::log_costs(unordered_map<MapCell*, int> cost_so_far, const Game& game) const
{
	log::log("Optimal path:");
	for (int y = 0; y < game.game_map->height; ++y)
	{
		string padding = (y <= 9) ? "0" : "";
		string line = "" + padding + to_string(y) + " | ";

		for (int x = 0; x < game.game_map->width; ++x)
			if (cost_so_far.count(&game.game_map->cells[y][x]))
				line += to_string(cost_so_far[&game.game_map->cells[y][x]]) + " ";
			else
				line += "0 ";

		log::log(line);
	}
	log::log("");
}

Position PathFinder::compute_direct_path(const Position& source_position, const Position& target_position, Game& game)
{
	Direction direction = game.game_map->get_move(source_position, target_position);
	return game.game_map->directional_offset(source_position, direction);
}

Position PathFinder::compute_shortest_path(const Position& source_position, const Position& target_position, Game& game)
{
	if ((source_position == target_position) || (game.distance(source_position, target_position) == 1))
		return target_position;

	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);

	//clock_t start = clock();
	vector<MapCell*> optimal_path = dijkstra_path(source_cell, target_cell, game);
	//log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	if (optimal_path.size() > 1)
		return optimal_path.at(1)->position;
	else
		return source_cell->position;
}

vector<MapCell*> PathFinder::adjacent_cells_filtered(MapCell* source_cell, MapCell* target_cell, MapCell* cell, const Game& game)
{
	// We add cells either if they are far (we expect things to move away from them),
	// or if they are marked as allies, or if it's target cell
	
	vector<MapCell*> adjacent_cells;

	Position north_position = game.game_map->directional_offset(cell->position, Direction::NORTH);
	int north_distance = game.game_map->calculate_distance(source_cell->position, north_position);
	if ((north_distance > 4) || (game.scorer.get_grid_score_move(north_position) < 9) || (north_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(north_position));

	Position south_position = game.game_map->directional_offset(cell->position, Direction::SOUTH);
	int south_distance = game.game_map->calculate_distance(source_cell->position, south_position);
	if ((south_distance > 4) || (game.scorer.get_grid_score_move(south_position) < 9) || (south_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(south_position));

	Position east_position = game.game_map->directional_offset(cell->position, Direction::EAST);
	int east_distance = game.game_map->calculate_distance(source_cell->position, east_position);
	if ((east_distance > 4) || (game.scorer.get_grid_score_move(east_position) < 9) || (east_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(east_position));

	Position west_position = game.game_map->directional_offset(cell->position, Direction::WEST);
	int west_distance = game.game_map->calculate_distance(source_cell->position, west_position);
	if ((west_distance > 4) || (game.scorer.get_grid_score_move(west_position) < 9) || (west_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(west_position));

	return adjacent_cells;
}

vector<MapCell*> PathFinder::adjacent_cells_all(MapCell* source_cell, MapCell* cell, const Game& game)
{
	vector<MapCell*> adjacent_cells;

	Position north_position = game.game_map->directional_offset(cell->position, Direction::NORTH);
	adjacent_cells.push_back(game.game_map->at(north_position));

	Position south_position = game.game_map->directional_offset(cell->position, Direction::SOUTH);
	adjacent_cells.push_back(game.game_map->at(south_position));

	Position east_position = game.game_map->directional_offset(cell->position, Direction::EAST);
	adjacent_cells.push_back(game.game_map->at(east_position));

	Position west_position = game.game_map->directional_offset(cell->position, Direction::WEST);
	adjacent_cells.push_back(game.game_map->at(west_position));

	return adjacent_cells;
}

vector<MapCell*> PathFinder::reconstruct_path(MapCell* source_cell, MapCell* target_cell, unordered_map<MapCell*, MapCell*> came_from)
{
	vector<MapCell*> path;
	MapCell* current = target_cell;
	while (current != source_cell) 
	{
		path.push_back(current);
		current = came_from[current];
	}
	path.push_back(source_cell);
	reverse(path.begin(), path.end());
	return path;
}

int PathFinder::compute_next_step_score(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, const Game& game) const
{
	int move_score = (int)floor(0.1 * current_cell->halite);

	// Free to move on highways!
	//move_score = (int)(move_score * game.scorer.get_grid_score_highway(current_cell->position));

	// Only apply bad score for enemies/allies if they are very close
	if (game.game_map->calculate_distance(source_cell->position, next_cell->position) <= game.get_constant("A* Radius Ships Seen"))
		move_score += (game.scorer.get_grid_score_move(next_cell->position) > 0) * 999999; //cannot put INT_MAX as it's going to be summed up after

	return move_score;
}

inline int PathFinder::heuristic(MapCell* cell, MapCell* target_cell, const Game& game) const
{
	return game.get_constant("A* Heuristic") * game.game_map->calculate_distance(cell->position, target_cell->position);
}

vector<MapCell*> PathFinder::dijkstra_path(MapCell* source_cell, MapCell* target_cell, const Game& game) const
{
	unordered_map<MapCell*, MapCell*> came_from;
	came_from[source_cell] = source_cell;

	PriorityQueue<MapCell*, int> frontier;
	frontier.put(source_cell, 0);

	unordered_map<MapCell*, int> cost_so_far;
	cost_so_far[source_cell] = 0;

	while (!frontier.empty())
	{
		// Pick the minimum distance unsettled cell
		MapCell* current_cell = frontier.get();

		// If current cell is the target, return its path
		if (current_cell->position == target_cell->position)
		{
			//if (game.turn_number == 10)
			//{
			//	log_costs(cost_so_far, game);
			//	log_path(reconstruct_path(source_cell, target_cell, came_from), game);
			//}
				
			return reconstruct_path(source_cell, target_cell, came_from);
		}

		//log::log("Current cell:" + current_cell->position.to_string_position());

		for (MapCell* next_cell : adjacent_cells_filtered(source_cell, target_cell, current_cell, game))
		{
			int new_cost = cost_so_far[current_cell] + compute_next_step_score(source_cell, current_cell, next_cell, game);

			if ( (cost_so_far.find(next_cell) == cost_so_far.end()) || (new_cost < cost_so_far[next_cell]) )
			{				
				cost_so_far[next_cell] = new_cost;
				came_from[next_cell] = current_cell;
				frontier.put(next_cell, new_cost + heuristic(next_cell, target_cell, game));
			}
		}
	}

	return vector<MapCell*>(1, source_cell);
}
//
//void set_cell(vector<vector<int>>& costs, MapCell* cell, int value)
//{
//	costs[cell->position.y][cell->position.x] = value;
//}
//
//int get_cell(vector<vector<int>>& costs, MapCell* cell)
//{
//	return costs[cell->position.y][cell->position.x];
//}
//vector<vector<int>> PathFinder::dijkstra_costs(MapCell* source_cell, const Game& game) const
//{
//	PriorityQueue<MapCell*, int> frontier;
//	frontier.put(source_cell, 0);
//
//	vector<vector<int>> costs(game.game_map->height, vector<int>(game.game_map->width, INT_MAX));
//	set_cell(costs, source_cell, 0);
//
//	while (!frontier.empty())
//	{
//		MapCell* current_cell = frontier.get();
//
//		for (MapCell* next_cell : adjacent_cells_all(source_cell, current_cell, game))
//		{
//			int new_cost = get_cell(costs, current_cell) + compute_next_step_score(source_cell, current_cell, next_cell, game);
//
//			if (new_cost < get_cell(costs, next_cell))
//			{
//				set_cell(costs, next_cell, new_cost);
//				frontier.put(next_cell, new_cost);
//			}
//		}
//	}
//
//	return costs;
//}
//
//vector<MapCell*> PathFinder::dijkstra_halite_per_turn(MapCell* source_cell, MapCell* target_cell, const Game& game) const
//{
//	unordered_map<MapCell*, MapCell*> came_from;
//	came_from[source_cell] = source_cell;
//
//	PriorityQueueInverted<MapCell*, int> frontier;
//	frontier.put(source_cell, 0);
//
//	unordered_map<MapCell*, int> halite_so_far;
//	halite_so_far[source_cell] = 0;
//
//	unordered_map<MapCell*, int> halite_per_turn_so_far;
//	halite_per_turn_so_far[source_cell] = 0;
//
//	unordered_map<MapCell*, int> turns_so_far;
//	turns_so_far[source_cell] = 0;
//
//	while (!frontier.empty())
//	{
//		MapCell* current_cell = frontier.get();
//
//		if (current_cell->position == target_cell->position)
//		{
//			//if (game.turn_number == 10)
//			//{
//				log_costs(halite_per_turn_so_far, game);
//				log_path(reconstruct_path(source_cell, target_cell, came_from), game);
//			//}
//
//			return reconstruct_path(source_cell, target_cell, came_from);
//		}
//
//		for (MapCell* next_cell : adjacent_cells_all(source_cell, current_cell, game))
//		{
//			int new_cost = halite_so_far[current_cell] + (int)ceil(0.25 * next_cell->halite);
//			int new_turn = turns_so_far[current_cell] + 1;
//			int new_halite_per_turn = new_cost / new_turn;
//
//			if (new_halite_per_turn > halite_per_turn_so_far[next_cell])
//			{
//				halite_so_far[next_cell] = new_cost;
//				turns_so_far[next_cell] = new_turn;
//				halite_per_turn_so_far[next_cell] = new_halite_per_turn;
//				frontier.put(next_cell, new_halite_per_turn - game.get_constant("A* Heuristic") * game.game_map->calculate_distance(next_cell->position, target_cell->position));
//			}
//		}
//	}
//
//	return vector<MapCell*>(1, source_cell);
//}
