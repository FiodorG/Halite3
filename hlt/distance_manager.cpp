#include "distance_manager.hpp"
#include "game.hpp"

#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace hlt;
using namespace std;

void DistanceManager::fill_closest_shipyard_or_dropoff(const Game& game)
{
	int width = game.game_map->width;
	int height = game.game_map->height;
	int radius = 3;

	// gather dropoffs
	vector<Position> base_or_dropoffs;
	base_or_dropoffs.push_back(game.my_shipyard_position());
	for (auto& dropoff_iterator : game.me->dropoffs)
		base_or_dropoffs.push_back(dropoff_iterator.second->position);

	// count enemies around each dropoff
	unordered_map<Position, int> enemies_around;
	for (auto& dropoff : base_or_dropoffs)
	{
		enemies_around[dropoff] = 0;

		for (int i = 0; i < height; ++i)
			for (int j = 0; j < width; ++j)
				if ((game.distance(Position(j, i), dropoff) <= radius) && (game.scorer.grid_score_move[i][j] == 10))
					enemies_around[dropoff] += 1;
		
		log::log("Dropoff: " + dropoff.to_string_position() + " has " + to_string(enemies_around[dropoff]) + " enemies around.");
	}

	// Find closest dropoff from each cell, discount by number of enemies around
	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			Position position = Position(j, i);
			Position closest_shipyard = Position();
			double min_distance = DBL_MAX;

			for (auto& dropoff : base_or_dropoffs)
			{
				double distance = (double)game.distance(position, dropoff);
				
				distance *= pow(1.1, max(enemies_around[dropoff] - 4, 0));

				if (enemies_around[dropoff] >= 10)
					distance = 9999999.0;

				if (distance <= min_distance)
				{
					min_distance = distance;
					closest_shipyard = dropoff;
				}
			}

			closest_shipyard_or_dropoff[i][j] = closest_shipyard;
			distance_cell_shipyard_or_dropoff[i][j] = game.distance(closest_shipyard, position);
		}

	//for (unsigned int i = 0; i < closest_shipyard_or_dropoff.size(); ++i)
	//{
	//	string padding = (i <= 9) ? "0" : "";
	//	string line = "" + padding + to_string(i) + " | ";
	//	for (unsigned int j = 0; j < closest_shipyard_or_dropoff.size(); ++j)
	//		line += closest_shipyard_or_dropoff[i][j].to_string_position() + " ";
	//	line += " | " + padding + to_string(i);

	//	padding = (i <= 9) ? "0" : "";
	//	line = "" + padding + to_string(i) + " | ";
	//	for (unsigned int j = 0; j < distance_cell_shipyard_or_dropoff.size(); ++j)
	//		line += to_string(distance_cell_shipyard_or_dropoff[i][j]) + " ";
	//	line += " | " + padding + to_string(i);

	//	hlt::log::log(line);
	//}
}