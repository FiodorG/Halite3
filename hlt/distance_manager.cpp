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
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			Position position = Position(j, i);
			Position shipyard_or_dropoff = game.get_closest_shipyard_or_dropoff(position);

			closest_shipyard_or_dropoff[i][j] = shipyard_or_dropoff;
			distance_cell_shipyard_or_dropoff[i][j] = game.distance(shipyard_or_dropoff, position);
		}

	//for (unsigned int i = 0; i < closest_shipyard_or_dropoff.size(); ++i)
	//{
	//	string padding = (i <= 9) ? "0" : "";
	//	string line = "" + padding + to_string(i) + " | ";
	//	for (unsigned int j = 0; j < closest_shipyard_or_dropoff.size(); ++j)
	//		line += closest_shipyard_or_dropoff[i][j].to_string_position() + " ";
	//	line += " | " + padding + to_string(i);

	//	hlt::log::log(line);
	//}
}