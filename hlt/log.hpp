#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

namespace hlt 
{
    namespace log 
	{
        void open(int bot_id);
        void log(const string& message);

		void log_vector(vector<int> vec);
		void log_vectorvector(vector<vector<int>> vec);
		void log_vectorvector(vector<vector<double>> vec);
    }
}
