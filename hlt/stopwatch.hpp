#pragma once

#include "defines.hpp"
#include "log.hpp"

#include <chrono>
#include <vector>
#include <string>

using namespace std;

namespace hlt
{
	class Stopwatch
	{
	public:
		Stopwatch(const string& identifier) : identifier(identifier)
		{
#if HALITE_LOCAL
			start = chrono::high_resolution_clock::now();
#endif
		};

		~Stopwatch()
		{
#if HALITE_LOCAL
			auto end = chrono::high_resolution_clock::now();
			chrono::duration<double> elapsed = end - start;
			long long ms = chrono::duration_cast<chrono::milliseconds>(elapsed).count();

			log::log(identifier + ": " + to_string(ms) + "ms");
#endif
		}

		string identifier;
		chrono::time_point<chrono::high_resolution_clock> start;
	};
}