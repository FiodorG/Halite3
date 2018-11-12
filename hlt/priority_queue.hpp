#pragma once

#include <vector>
#include <queue>

using namespace std;

namespace hlt 
{
	template<typename T, typename priority_t> struct PriorityQueue
	{
		typedef pair<priority_t, T> PQElement;
		priority_queue<PQElement, vector<PQElement>, greater<PQElement>> elements;

		inline bool empty() const 
		{
			return elements.empty();
		}

		inline void put(T item, priority_t priority) 
		{
			elements.emplace(priority, item);
		}

		T get() 
		{
			T best_item = elements.top().second;
			elements.pop();
			return best_item;
		}
	};

	template<typename T, typename priority_t> struct PriorityQueueInverted
	{
		typedef pair<priority_t, T> PQElement;
		priority_queue<PQElement, vector<PQElement>, less<PQElement>> elements;

		inline bool empty() const
		{
			return elements.empty();
		}

		inline void put(T item, priority_t priority)
		{
			elements.emplace(priority, item);
		}

		T get()
		{
			T best_item = elements.top().second;
			elements.pop();
			return best_item;
		}
	};
}
