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

	constants["Score: Smoothing radius"] = 3;

	constants["Test"] = 0;

	return mybot_internal("GSBot1", constants, rng_seed);
}
