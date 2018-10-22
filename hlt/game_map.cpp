#include "game_map.hpp"
#include "game.hpp"
#include "input.hpp"

#include <cfloat>
#include <limits>

using namespace hlt;
using namespace std;

double GameMap::scoring_function(const MapCell& source_cell, const MapCell& target_cell, const Game& game)
{
	// Too low ressource on cell
	if (target_cell.halite < 40)
		return -DBL_MAX;

	// Enemy on cell
	if (game.enemy_in_cell(target_cell))
		return -DBL_MAX;

	int distance = calculate_distance(target_cell.position, source_cell.position);
	int d_halite = target_cell.halite - source_cell.halite;

	if (d_halite > 0)
		return (double)d_halite / (1 + distance) / (1 + distance);
	else
		return (double)d_halite;
}

MapCell* GameMap::closest_cell_with_ressource(const Entity& entity, const Game& game)
{
	MapCell* optimal_cell = at(entity.position);
	double optimal_score = scoring_function(*at(entity.position), *at(entity.position), game);
	double new_score = 0;

	for (vector<MapCell>& row : cells)
		for (MapCell& cell : row)
		{
			if (!game.existing_objective_to_cell(cell))
			{
				new_score = scoring_function(*at(entity.position), cell, game);
				if (optimal_score < new_score)
				{
					optimal_cell = &cell;
					optimal_score = new_score;
				}
			}
		}

	return optimal_cell;
}

void GameMap::_update() 
{
    for (int y = 0; y < height; ++y) 
		for (int x = 0; x < width; ++x)
			cells[y][x].ship.reset();

    int update_count;
    get_sstream() >> update_count;

    for (int i = 0; i < update_count; ++i) 
	{
        int x;
        int y;
        int halite;
        get_sstream() >> x >> y >> halite;
        cells[y][x].halite = halite;
    }
}

unique_ptr<GameMap> GameMap::_generate() 
{
    unique_ptr<GameMap> map = make_unique<GameMap>();

    get_sstream() >> map->width >> map->height;
    map->cells.resize((size_t)map->height);

    for (int y = 0; y < map->height; ++y) 
	{
        auto in = get_sstream();

        map->cells[y].reserve((size_t)map->width);

        for (int x = 0; x < map->width; ++x) 
		{
            Halite halite;
            in >> halite;

            map->cells[y].push_back(MapCell(x, y, halite));
        }
    }

    return map;
}
