#include "distance_manager.hpp"
#include "game.hpp"

#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace hlt;
using namespace std;

//void DistanceManager::fill_distances(const Game& game)
//{
//	ship_distances.clear();
//	for (const auto& ship : game.me->ships)
//		ship_distances[ship.second] = game.pathfinder.dijkstra_costs(game.mapcell(ship.second), game);
//
//	shipyard_or_dropoff_distances.clear();
//	for (const auto& dropoff : game.me->dropoffs)
//		shipyard_or_dropoff_distances[dropoff.second->position] = game.pathfinder.dijkstra_costs(game.mapcell(dropoff.second->position), game);
//
//	shipyard_or_dropoff_distances[game.me->shipyard->position] = game.pathfinder.dijkstra_costs(game.mapcell(game.me->shipyard->position), game);
//}