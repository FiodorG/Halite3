#include "scorer.hpp"
#include "game.hpp"

#include <cmath>

using namespace hlt;
using namespace std;

void hlt::Scorer::update_grid_score(const Game& game)
{
	for (int i = 0; i < game.game_map->height; ++i)
		for (int j = 0; j < game.game_map->width; ++j)
		{
			grid_score[i][j] = 0;

			// If enemy is in contiguous cell, bad score. Not use UINT_MIN not to cause overflow errors.
			if (
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::NORTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::SOUTH)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::EAST)) ||
				game.enemy_in_cell(game.game_map->directional_offset(Position(j, i), Direction::WEST)) ||
				game.game_map->cells[i][j].is_occupied_by_enemy(game.my_id)
				)
				grid_score[i][j] = 9;
		}
}

void hlt::Scorer::add_self_ships_to_grid_score(shared_ptr<Ship> ship, const Position& position)
{
	grid_score[position.y][position.x] += 1;
}

void hlt::Scorer::flush_grid_score(const Position& position)
{
	grid_score[position.y][position.x] = max(0, grid_score[position.y][position.x] - 1);
}
