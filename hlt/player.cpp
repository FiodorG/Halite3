#include "player.hpp"
#include "input.hpp"

using namespace hlt;
using namespace std;

void Player::_update(int num_ships, int num_dropoffs, Halite halite) 
{
    this->halite = halite;

	unordered_map<EntityId, shared_ptr<Ship>> old_ships = ships;

    ships.clear();
    for (int i = 0; i < num_ships; ++i) 
	{
		shared_ptr<Ship> new_ship = Ship::_generate(id);

		if (old_ships.find(new_ship->id) != old_ships.end())
		{
			shared_ptr<Ship> old_ship = old_ships[new_ship->id];
			new_ship->assign_objective(old_ship);
		}

        ships[new_ship->id] = new_ship;
    }
	old_ships.clear();

    dropoffs.clear();
    for (int i = 0; i < num_dropoffs; ++i) 
	{
        shared_ptr<Dropoff> dropoff = Dropoff::_generate(id);
        dropoffs[dropoff->id] = dropoff;
    }
}

shared_ptr<Player> Player::_generate() 
{
    PlayerId player_id;
    int shipyard_x;
    int shipyard_y;
    get_sstream() >> player_id >> shipyard_x >> shipyard_y;

    return make_shared<Player>(player_id, shipyard_x, shipyard_y);
}
