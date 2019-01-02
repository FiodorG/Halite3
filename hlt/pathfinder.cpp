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

Position PathFinder::compute_path(shared_ptr<Ship> ship, const Position& position, Game& game)
{
	Position target_position;

	if (ship->is_objective(Objective_Type::ATTACK))
		target_position = compute_direct_path(ship->position, position, game);
	else if (ship->is_objective(Objective_Type::BLOCK_ENEMY_BASE))
		target_position = compute_direct_path_no_base(ship->position, position, game);
	else if (ship->is_objective(Objective_Type::SUICIDE_ON_BASE))
		target_position = compute_direct_path_suicide(ship->position, position, game);
	else if (ship->is_objective(Objective_Type::BACK_TO_BASE))
		target_position = compute_direct_path_rtb(ship->position, position, game);
	else
		target_position = compute_shortest_path(ship->position, position, game);

	return target_position;
}

Position PathFinder::compute_direct_path_rtb(const Position& source_position, const Position& target_position, Game& game)
{
	if ((source_position == target_position) || (game.distance(source_position, target_position) == 1))
		return target_position;

	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);

	//clock_t start = clock();
	vector<MapCell*> optimal_path = dijkstra_rtb(source_cell, target_cell, game);
	//log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	if (optimal_path.size() > 1)
	{
		return optimal_path.at(1)->position;

		/*Position next_position = optimal_path.at(1)->position;
		if (game.distance(source_position, target_position) < game.distance(next_position, target_position))
			return source_position;*/
	}
	else
		return source_position;
}

Position PathFinder::compute_direct_path_no_base(const Position& source_position, const Position& target_position, Game& game)
{
	Position enemy_base = game.get_closest_enemy_shipyard_or_dropoff(target_position);

	if ((source_position == target_position) || (game.distance(source_position, target_position) == 1))
		return target_position;

	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);
	MapCell* enemy_base_cell = game.game_map->at(enemy_base);

	//clock_t start = clock();
	vector<MapCell*> optimal_path = dijkstra_block(source_cell, target_cell, enemy_base_cell, game);
	//log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	if (optimal_path.size() > 1)
		return optimal_path.at(1)->position;
	else
		return source_cell->position;
}

Position PathFinder::compute_direct_path_suicide(const Position& source_position, const Position& target_position, Game& game)
{
	if ((source_position == target_position) || (game.distance(source_position, target_position) == 1))
		return target_position;

	MapCell* source_cell = game.game_map->at(source_position);
	MapCell* target_cell = game.game_map->at(target_position);

	//clock_t start = clock();
	vector<MapCell*> optimal_path = dijkstra_suicide(source_cell, target_cell, target_cell, game);
	//log::log("Dijkstra for " + source_position.to_string_position() + " to " + target_position.to_string_position() + " took: " + to_string((clock() - start) / (double)CLOCKS_PER_SEC));

	if (optimal_path.size() > 1)
		return optimal_path.at(1)->position;
	else
		return source_cell->position;
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

vector<MapCell*> PathFinder::adjacent_cells_all(MapCell* cell, const Game& game)
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

	// Only apply bad score for enemies/allies if they are very close
	if (game.distance(source_cell->position, next_cell->position) <= game.get_constant("A* Radius Ships Seen"))
		move_score += (game.scorer.get_grid_score_move(next_cell->position) > 0) * 999999; //cannot put INT_MAX as it's going to be summed up after

	return move_score;
}

int PathFinder::compute_next_step_score_rtb(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, const Game& game) const
{
	int move_score = (int)floor(0.1 * current_cell->halite);

	// Only apply bad score for enemies/allies if they are very close
	int distance = game.distance(source_cell->position, next_cell->position);
	if (distance <= 4)
		move_score += (int)((game.scorer.get_grid_score_move(next_cell->position) > 2) * 100.0 * (4.0 - (double)distance) / 4.0);

	// do not go straight on enemy cell
	if (distance <= 1)
		move_score += (game.scorer.get_grid_score_move(next_cell->position) == 10) * 9999999;

	return move_score;
}

inline int PathFinder::heuristic(MapCell* cell, MapCell* target_cell, const Game& game) const
{
	return game.get_constant("A* Heuristic") * game.game_map->calculate_distance(cell->position, target_cell->position);
}

vector<MapCell*> PathFinder::dijkstra_rtb(MapCell* source_cell, MapCell* target_cell, const Game& game) const
{
	unordered_map<MapCell*, MapCell*> came_from;
	came_from[source_cell] = source_cell;

	PriorityQueue<MapCell*, int> frontier;
	frontier.put(source_cell, 0);

	unordered_map<MapCell*, int> cost_so_far;
	cost_so_far[source_cell] = 0;

	while (!frontier.empty())
	{
		MapCell* current_cell = frontier.get();

		if (current_cell->position == target_cell->position)
			return reconstruct_path(source_cell, target_cell, came_from);

		for (MapCell* next_cell : adjacent_cells_all(current_cell, game))
		{
			int new_cost = cost_so_far[current_cell] + compute_next_step_score_rtb(source_cell, current_cell, next_cell, game);

			if ((cost_so_far.find(next_cell) == cost_so_far.end()) || (new_cost < cost_so_far[next_cell]))
			{
				cost_so_far[next_cell] = new_cost;
				came_from[next_cell] = current_cell;
				frontier.put(next_cell, new_cost + heuristic(next_cell, target_cell, game));
			}
		}
	}

	return vector<MapCell*>(1, source_cell);
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
		MapCell* current_cell = frontier.get();

		if (current_cell->position == target_cell->position)
			return reconstruct_path(source_cell, target_cell, came_from);

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

int PathFinder::compute_next_step_score_block(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, MapCell* enemy_base, const Game& game) const
{
	int move_score = 1;

	if (next_cell->has_structure())
		move_score = 4;

	if (game.game_map->calculate_distance_inf(next_cell->position, enemy_base->position) == 1)
		move_score = 0;

	if ((game.scorer.get_grid_score_move(next_cell->position) > 0) && (game.scorer.get_grid_score_move(next_cell->position) < 9))
		move_score = 10;

	if (game.scorer.get_grid_score_move(next_cell->position) == 10)
		move_score = 999;

	return move_score;
}

vector<MapCell*> PathFinder::dijkstra_block(MapCell* source_cell, MapCell* target_cell, MapCell* enemy_base, const Game& game) const
{
	unordered_map<MapCell*, MapCell*> came_from;
	came_from[source_cell] = source_cell;

	PriorityQueue<MapCell*, int> frontier;
	frontier.put(source_cell, 0);

	unordered_map<MapCell*, int> cost_so_far;
	cost_so_far[source_cell] = 0;

	while (!frontier.empty())
	{
		MapCell* current_cell = frontier.get();

		if (current_cell->position == target_cell->position)
			return reconstruct_path(source_cell, target_cell, came_from);

		//log::log("Current cell:" + current_cell->position.to_string_position());

		for (MapCell* next_cell : adjacent_cells_all(current_cell, game))
		{
			int new_cost = cost_so_far[current_cell] + compute_next_step_score_block(source_cell, current_cell, next_cell, enemy_base, game);

			if ((cost_so_far.find(next_cell) == cost_so_far.end()) || (new_cost < cost_so_far[next_cell]))
			{
				cost_so_far[next_cell] = new_cost;
				came_from[next_cell] = current_cell;
				frontier.put(next_cell, new_cost + heuristic(next_cell, target_cell, game));
			}
		}
	}

	return vector<MapCell*>(1, source_cell);
}

int PathFinder::compute_next_step_score_suicide(MapCell* source_cell, MapCell* current_cell, MapCell* next_cell, MapCell* base, const Game& game) const
{
	int move_score = 1;

	if (game.game_map->calculate_distance(source_cell->position, next_cell->position) <= game.get_constant("A* Radius Ships Seen"))
		move_score += (game.scorer.get_grid_score_move(next_cell->position) > 0) * 999999;

	if (
		(game.game_map->calculate_distance(source_cell->position, base->position) <= 4) ||
		(game.turns_remaining() <= 8)
	)
		if (game.scorer.get_grid_score_move(next_cell->position) >= 9)
			move_score = 1;

	return move_score;
}

vector<MapCell*> PathFinder::dijkstra_suicide(MapCell* source_cell, MapCell* target_cell, MapCell* base, const Game& game) const
{
	unordered_map<MapCell*, MapCell*> came_from;
	came_from[source_cell] = source_cell;

	PriorityQueue<MapCell*, int> frontier;
	frontier.put(source_cell, 0);

	unordered_map<MapCell*, int> cost_so_far;
	cost_so_far[source_cell] = 0;

	while (!frontier.empty())
	{
		MapCell* current_cell = frontier.get();

		if (current_cell->position == target_cell->position)
			return reconstruct_path(source_cell, target_cell, came_from);

		for (MapCell* next_cell : adjacent_cells_all(current_cell, game))
		{
			int new_cost = cost_so_far[current_cell] + compute_next_step_score_suicide(source_cell, current_cell, next_cell, base, game);

			if ((cost_so_far.find(next_cell) == cost_so_far.end()) || (new_cost < cost_so_far[next_cell]))
			{
				cost_so_far[next_cell] = new_cost;
				came_from[next_cell] = current_cell;
				frontier.put(next_cell, new_cost + heuristic(next_cell, target_cell, game));
			}
		}
	}

	return vector<MapCell*>(1, source_cell);
}