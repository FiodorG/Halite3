#include "pathfinder.hpp"
#include "game.hpp"

#include <cmath>

using namespace hlt;
using namespace std;

Position PathFinder::compute_shortest_path(const Position& source_position, const Position& target_position, Game& game)
{
	if (source_position == target_position)
		return source_position;

	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);

	//clock_t start = clock();
	vector<MapCell*> optimal_path = dijkstra(source_cell, target_cell, game);
	//log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	//for (Position& pos : optimal_path)
	//	log::log(pos.to_string_position());

	if (optimal_path.size() > 1)
	{
		log::log("Dijkstra - Source: " + source_cell->position.to_string_position() + ", Target: " + target_cell->position.to_string_position() + ", Moving to " + optimal_path.at(1)->position.to_string_position());
		return optimal_path.at(1)->position;
	}
	else
		return source_cell->position;
}

vector<MapCell*> PathFinder::adjacent_cells(MapCell* source_cell, MapCell* target_cell, MapCell* cell, const Game& game)
{
	// We add cells either if they are far (we expect things to move away from them),
	// or if they are marked as allies, or if it's target cell
	
	vector<MapCell*> adjacent_cells;

	Position north_position = game.game_map->directional_offset(cell->position, Direction::NORTH);
	int north_distance = game.game_map->calculate_distance(source_cell->position, north_position);
	if ((north_distance > 4) || (game.scorer.get_grid_score(north_position) < 9) || (north_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(north_position));

	Position south_position = game.game_map->directional_offset(cell->position, Direction::SOUTH);
	int south_distance = game.game_map->calculate_distance(source_cell->position, south_position);
	if ((south_distance > 4) || (game.scorer.get_grid_score(south_position) < 9) || (south_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(south_position));

	Position east_position = game.game_map->directional_offset(cell->position, Direction::EAST);
	int east_distance = game.game_map->calculate_distance(source_cell->position, east_position);
	if ((east_distance > 4) || (game.scorer.get_grid_score(east_position) < 9) || (east_position == target_cell->position))
		adjacent_cells.push_back(game.game_map->at(east_position));

	Position west_position = game.game_map->directional_offset(cell->position, Direction::WEST);
	int west_distance = game.game_map->calculate_distance(source_cell->position, west_position);
	if ((west_distance > 4) || (game.scorer.get_grid_score(west_position) < 9) || (west_position == target_cell->position))
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

int PathFinder::compute_next_step_score(MapCell* source_cell, MapCell* next_cell, const Game& game) const
{
	int move_score = (int)floor(0.1 * source_cell->halite);

	// Only apply bad score for enemies/allies if they are very close
	if (game.game_map->calculate_distance(source_cell->position, next_cell->position)<5)
		move_score += (game.scorer.grid_score[next_cell->position.y][next_cell->position.x]>0) * 9999999;

	return move_score;
}

inline int PathFinder::heuristic(MapCell* cell, MapCell* target_cell, Game& game) const
{
	return game.get_constant("A* Heuristic") * game.game_map->calculate_distance(cell->position, target_cell->position);
}

vector<MapCell*> PathFinder::dijkstra(MapCell* source_cell, MapCell* target_cell, Game& game)
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

		// If current cell is the target, return it's path
		if (current_cell->position == target_cell->position)
			return reconstruct_path(source_cell, target_cell, came_from);

		//log::log("Current cell:" + current_cell->position.to_string_position());

		for (MapCell* next_cell : adjacent_cells(source_cell, target_cell, current_cell, game))
		{
			int new_cost = cost_so_far[current_cell] + compute_next_step_score(source_cell, next_cell, game);

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