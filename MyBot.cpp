#include "hlt/mybot_internal.hpp"

#include <random>
#include <unordered_map>
#include <string>

using namespace std;

int main(int argc, char* argv[]) 
{
    unsigned int rng_seed;
    if (argc > 1)
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
	else 
        rng_seed = static_cast<unsigned int>(time(nullptr));
	mt19937 rng(rng_seed);

	/* Constants */
	unordered_map<string, int> constants;
	constants["A* Heuristic"] = 20;
	constants["A* Radius Ships Seen"] = 1;

	constants["Max Ships 2p: 32"] = 26;
	constants["Max Ships 2p: 40"] = 32;
	constants["Max Ships 2p: 48"] = 45;
	constants["Max Ships 2p: 56"] = 50;
	constants["Max Ships 2p: 64"] = 55;

	constants["Score: Brute force reach"] = 4;
	constants["Score: Smoothing radius"] = 3;
	constants["Score: Inspiration Bonus"] = 2;
	constants["Score: Remove Halite Multiplier"] = 4;

	constants["Dropoff: No Go Zone"] = 7;

	constants["Test"] = 0;

	return mybot_internal("GSBot1", constants, rng_seed);
}
