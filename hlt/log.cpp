#include "log.hpp"
#include "defines.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <stdio.h>

static std::ofstream log_file;
static std::vector<std::string> log_buffer;
static bool has_opened = false;
static bool has_atexit = false;

void dump_buffer_at_exit() 
{
    if (has_opened) 
        return;

    auto now_in_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    std::string filename = "bot-unknown-" + std::to_string(now_in_nanos) + ".log";
    std::ofstream file(filename, std::ios::trunc | std::ios::out);
    for (const std::string& message : log_buffer) {
        file << message << std::endl;
    }
}

void hlt::log::open(int bot_id) 
{
    if (has_opened) 
	{
        hlt::log::log("Error: log: tried to open(" + std::to_string(bot_id) + ") but we have already opened before.");
        exit(1);
    }

    has_opened = true;
    std::string filename = "bot-" + std::to_string(bot_id) + ".log";
    log_file.open(filename, std::ios::trunc | std::ios::out);

    for (const std::string& message : log_buffer) 
	{
        log_file << message << std::endl;
    }
    log_buffer.clear();
}

void hlt::log::log(const std::string& message) 
{
    if (has_opened) 
	{
		#if HALITE_LOCAL
        log_file << message << std::endl;
		#endif
    } 
	else 
	{
        if (!has_atexit) 
		{
            has_atexit = true;
            atexit(dump_buffer_at_exit);
        }
        log_buffer.push_back(message);
    }
}

void hlt::log::log_vector(std::vector<int> vec)
{
	string line = "";
	for (unsigned int j = 0; j < vec.size(); ++j)
		line += to_string(vec[j]) + " ";

	hlt::log::log(line);
}

void hlt::log::log_vectorvector(std::vector<std::vector<int>> vec)
{
	for (unsigned int i = 0; i < vec.size(); ++i)
	{
		string padding = (i <= 9) ? "0" : "";
		string line = "" + padding + to_string(i) + " | ";
		for (unsigned int j = 0; j < vec.size(); ++j)
			line += to_string(vec[i][j]) + " ";
		line += " | " + padding + to_string(i);

		hlt::log::log(line);
	}

	hlt::log::log("");
}

void hlt::log::log_vectorvector(std::vector<std::vector<double>> vec)
{
	for (unsigned int i = 0; i < vec.size(); ++i)
	{
		string padding = (i <= 9) ? "0" : "";
		string line = "" + padding + to_string(i) + " | ";
		for (unsigned int j = 0; j < vec.size(); ++j)
			line += to_string(vec[i][j]) + " ";
		line += " | " + padding + to_string(i);

		hlt::log::log(line);
	}

	hlt::log::log("");
}