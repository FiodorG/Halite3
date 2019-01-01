#include "blocker.hpp"
#include "game.hpp"

#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace hlt;
using namespace std;


Objective Blocker::find_best_objective_cell(shared_ptr<Ship> ship, const Game& game) const
{
	double max_score = -DBL_MAX;
	Position max_position = Position();

	for (auto& enemy_base_map : positions_to_block_scores)
	{
		for (auto& position_score : enemy_base_map.second)
		{
			Position position = position_score.first;
			double score = position_score.second;
			double distance = (double)game.distance(ship->position, position);
			double total_score;

			if (score > 0)
				total_score = score / max(1.0, distance);
			else
				total_score = score - distance * 200;

			if (total_score > max_score)
			{
				max_score = total_score;
				max_position = position;
			}
		}
	}

	return Objective(-1, Objective_Type::BLOCK_ENEMY_BASE, max_position, max_score);
}

void Blocker::decrease_score_in_position(const Position& position, const Game& game)
{
	for (auto& enemy_base_map : positions_to_block_scores)
		for (auto& position_score : enemy_base_map.second)
			if (position_score.first == position)
			{
				positions_to_block_scores[enemy_base_map.first][position] -= 500.0;
				return;
			}
}

void Blocker::fill_positions_to_block_scores(const Game& game)
{
	positions_to_block_scores.clear();

	for (auto& enemy_base : game.enemy_shipyard_or_dropoff_positions())
		positions_to_block_scores[enemy_base] = position_to_block_on_enemy_base(enemy_base, game);

	log::log("Block Enemy bases");
	for (auto& enemy_base_map : positions_to_block_scores)
	{
		string line = "Enemy Base: " + enemy_base_map.first.to_string_position();
		for (auto& position_score : enemy_base_map.second)
			line += ", " + position_score.first.to_string_position() + ": " + to_string(position_score.second);
		log::log(line);
	}
}

unordered_map<Position, double> Blocker::position_to_block_on_enemy_base(const Position& enemy_base, const Game& game) const
{
	Position north = game.game_map->directional_offset(enemy_base, Direction::NORTH);
	Position south = game.game_map->directional_offset(enemy_base, Direction::SOUTH);
	Position east = game.game_map->directional_offset(enemy_base, Direction::EAST);
	Position west = game.game_map->directional_offset(enemy_base, Direction::WEST);

	double score_north = 0.0, score_south = 0.0, score_east = 0.0, score_west = 0.0;

	int width = game.game_map->width;
	int height = game.game_map->height;
	int reach = 6;

	vector<vector<int>> grid = vector<vector<int>>(height, vector<int>(width, 0));

	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			grid[i][j] = 0;

			Position position = Position(j, i);

			if (game.distance(position, enemy_base) > reach)
				continue;

			const int dx = position.x - enemy_base.x;
			const int dy = position.y - enemy_base.y;

			int halite_enemy = 0;
			int halite_ally = 0;

			if (game.enemy_in_cell(position) && (game.ship_on_position(position)->owner == game.mapcell(enemy_base)->structure->owner))
				halite_enemy += game.mapcell(position)->ship->halite;

			if (game.ally_in_cell(position))
				halite_ally += 500;

			if (abs(dx) == abs(dy)) // on a diagonal
			{
				if ((dx == 0) && (dy == 0)) // enemy base
				{
					grid[i][j] = 0;
				}
				else if ((dx < 0) && (dy < 0)) // north west
				{
					grid[i][j] = 1;

					score_north += 0.5 * (halite_enemy - halite_ally);
					score_west += 0.5 * (halite_enemy - halite_ally);
				}
				else if ((dx > 0) && (dy < 0)) // north est
				{
					grid[i][j] = 1;

					score_north += 0.5 * (halite_enemy - halite_ally);
					score_east += 0.5 * (halite_enemy - halite_ally);
				}
				else if ((dx < 0) && (dy > 0)) // south west
				{
					grid[i][j] = 2;

					score_south += 0.5 * (halite_enemy - halite_ally);
					score_west += 0.5 * (halite_enemy - halite_ally);
				}
				else // south east
				{
					grid[i][j] = 2;

					score_south += 0.5 * (halite_enemy - halite_ally);
					score_east += 0.5 * (halite_enemy - halite_ally);
				}
			}
			else if (abs(dx) > abs(dy)) // either east or west
			{
				if (dx < 0) // west
				{
					grid[i][j] = 3;
					score_west += halite_enemy - halite_ally;
				}
				else // east
				{
					grid[i][j] = 4;
					score_east += halite_enemy - halite_ally;
				}
			}
			else // either north or south
			{
				if (dy < 0) // north
				{
					grid[i][j] = 1;
					score_north += halite_enemy - halite_ally;
				}
				else // south
				{
					grid[i][j] = 2;
					score_south += halite_enemy - halite_ally;
				}
			}	
		} 

	unordered_map<Position, double> scores;
	scores[north] = score_north;
	scores[south] = score_south;
	scores[east] = score_east;
	scores[west] = score_west;

	//string line = "";
	//for (int i = 0; i < 4; i++)
	//	line += "" + positions[i].to_string_position() + ": " + to_string(scores[i]) + ", ";
	//log::log(line);

	//log::log_vectorvector(grid);

	return scores;
}