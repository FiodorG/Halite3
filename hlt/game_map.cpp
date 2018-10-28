#include "game_map.hpp"
#include "game.hpp"
#include "input.hpp"

#include <cfloat>
#include <limits>
#include <cmath>

using namespace hlt;
using namespace std;

double GameMap::scoring_function(MapCell* source_cell, MapCell* target_cell, const Game& game)
{
	int distance = calculate_distance(target_cell->position, source_cell->position);

	// Enemy on cell
	if ((distance < 5) && game.enemy_in_adjacent_cell(target_cell->position))
		return -DBL_MAX;

	// Ally on cell
	if ((distance < 3) && game.ally_in_cell(target_cell->position))
		return -DBL_MAX;

	int halite = target_cell->halite;
	if (halite < 40)
		halite = (int)((double)halite * 0.1);

	return halite / pow((double)(1 + distance), 2.0 - game.turn_percent() * 1.5 );
}

MapCell* GameMap::closest_cell_with_ressource(shared_ptr<Ship> ship, const Game& game)
{
	MapCell* optimal_cell = at(ship->position);
	double optimal_score = scoring_function(optimal_cell, optimal_cell, game);
	double new_score = 0;

	for (vector<MapCell>& row : cells)
		for (MapCell& cell : row)
		{
			if (!game.existing_objective_to_cell(cell) && (cell.position != ship->position))
			{
				new_score = scoring_function(at(ship->position), &cell, game);
				if (optimal_score < new_score)
				{
					optimal_cell = &cell;
					optimal_score = new_score;
				}
			}
		}

	log::log(ship->to_string_ship() + " allocated to " + optimal_cell->position.to_string_position() + " with score " + to_string(optimal_score));
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
