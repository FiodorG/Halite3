#include "game_grid.hpp"
#include "game.hpp"

#include <cmath>

using namespace hlt;
using namespace std;

Position GameGrid::compute_shortest_path(const Position& source_position, const Position& target_position, const Game& game) const
{
	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);

	//log::log("Source: " + source_cell->position.to_string_position() + ", Target: " + target_cell->position.to_string_position());

	clock_t start = clock();
	list<Position> optimal_path = dijkstra(source_cell, target_cell, game);
	log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	//for (Position& pos : optimal_path)
	//	log::log(pos.to_string_position());

	if (optimal_path.size() > 1)
		return *(next(optimal_path.begin()));
	else
		return source_cell->position;
}

MapCell* GameGrid::get_lowest_distance_cell(const list<MapCell*>& unsettled_cells, const list<MapCell*>& settled_cells, const Game& game)
{
	// Make sure we return source on first loop
	if ((unsettled_cells.size() == 1) && (settled_cells.size() == 0))
		return unsettled_cells.front();

	double closest_distance = DBL_MAX;
	MapCell* closest_cell;

	for (MapCell* settled_cell : settled_cells)
	{
		for (MapCell* unsettled_cell : unsettled_cells)
		{
			int distance = game.game_map->calculate_distance(unsettled_cell->position, settled_cell->position);
			if (distance < closest_distance)
			{
				closest_distance = distance;
				closest_cell = unsettled_cell;
			}
		}
	}

	return closest_cell;
}

vector<MapCell*> GameGrid::adjacent_cells(MapCell* cell, const Game& game)
{
	vector<MapCell*> adjacent_cells;

	adjacent_cells.push_back(game.game_map->at(game.game_map->directional_offset(cell->position, Direction::NORTH)));
	adjacent_cells.push_back(game.game_map->at(game.game_map->directional_offset(cell->position, Direction::SOUTH)));
	adjacent_cells.push_back(game.game_map->at(game.game_map->directional_offset(cell->position, Direction::EAST)));
	adjacent_cells.push_back(game.game_map->at(game.game_map->directional_offset(cell->position, Direction::WEST)));

	return adjacent_cells;
}

list<Position> GameGrid::dijkstra(MapCell* source_cell, MapCell* target_cell, const Game& game) const
{
	// FIXME could replace vector of lists by came_from
	// FIXME or use A*
	// FIXME or only restrict Dijkstra to box with corners being source and target

	int N = width * width;

	// The output array. distances[i] will hold the shortest distance from source to i 
	vector<int> distances(N, INT_MAX);
	distances[to_index(source_cell)] = 0;

	// settled_positions[i] = true if vertex i is included in shortest path tree or shortest distance from src to i is finalized 
	list<MapCell*> unsettled_cells;
	list<MapCell*> settled_cells;
	unsettled_cells.push_back(source_cell);

	// Parent array to store shortest path tree 
	vector<list<Position>> shortest_path(N, list<Position>());

	// Find shortest path until target_cell is settled
	while (true)
	{
		// Pick the minimum distance unsettled cell
		MapCell* current_cell = get_lowest_distance_cell(unsettled_cells, settled_cells, game);

		// If current cell is the target, return it's path
		if (current_cell->position == target_cell->position)
		{
			shortest_path[to_index(target_cell)].push_back(target_cell->position);
			return shortest_path[to_index(target_cell)];
		}

		unsettled_cells.remove(current_cell);
		int index_current = to_index(current_cell);

		//log::log("Current cell:" + current_cell->position.to_string_position());
		//log::log("Target cell:" + target_cell->position.to_string_position());

		for (MapCell* adjacent_cell : adjacent_cells(current_cell, game))
		{
			int index_adjacent = to_index(adjacent_cell);

			if (find(settled_cells.begin(), settled_cells.end(), adjacent_cell) == settled_cells.end())
			{
				int source_distance   = distances[index_current];
				int adjacent_distance = distances[index_adjacent];
				int edge_weight       = (int)ceil(0.1 * current_cell->halite);

				//log::log("Adjacent: " + adjacent_cell->position.to_string_position() + ": " + to_string(source_distance) + ", " + to_string(adjacent_distance) + ", " + to_string(edge_weight));

				if (source_distance + edge_weight < adjacent_distance)
				{
					distances[index_adjacent] = source_distance + edge_weight;
					shortest_path[index_adjacent] = shortest_path[index_current];
					shortest_path[index_adjacent].push_back(current_cell->position);
				}

				unsettled_cells.push_back(adjacent_cell);
			}
		}
		settled_cells.push_back(current_cell);

		//log::log("Settled: ");
		//for (MapCell* pos : settled_cells)
		//	log::log(pos->position.to_string_position());

		//log::log("Unsettled: ");
		//for (MapCell* pos : unsettled_cells)
		//	log::log(pos->position.to_string_position());

		//if (shortest_path[index_current].size())
		//{
		//	log::log("Shortest path of " + current_cell->position.to_string_position());
		//	for (Position& pos : shortest_path[index_current])
		//		log::log(pos.to_string_position());
		//}
	}
}