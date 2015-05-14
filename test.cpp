/**
* Implementation of Naive and Lock-free ring-buffer queues and
* performance and verification tests.
*
* Build with (g++ version must be >= 4.5.0):
* $ g++ -Wall -std=c++0x -Wl,--no-as-needed -O2 -D DCACHE1_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE` lockfree_rb_q.cc -lpthread
*
* I verified the program with g++ 4.5.3, 4.6.1, 4.6.3 and 4.8.1.
*
* Use -std=c++11 instead of -std=c++0x for g++ 4.8.
*
* Copyright (C) 2012-2013 Alexander Krizhanovsky (ak@natsys-lab.com).
*
* This file is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation; either version 3, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
* See http://www.gnu.org/licenses/lgpl.html .
*/

#include <iostream>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <chrono>

#include "lockless_queue.h"

threading::lockless_queue<std::string>* g_s;

void writeSomething(std::string what, int l_itemCount)
{
	for (; l_itemCount > 0; --l_itemCount)
	{
		g_s->produce(std::move(what));
	}
}

void getSomething(std::atomic<bool>& running)
{


	std::shared_ptr<std::string> result;
	do
	{
		result = g_s->consume(false);
	} while (result || running);

}

int main()
{
	int l_minThreadCount, l_maxThreadCount;
	int l_minItemCount, l_maxItemCount, l_itemCountIncrement;
	std::cout << "Minimum number of threads?:";
	std::cin >> l_minThreadCount;
	std::cout << "Minimum number of threads?:";
	std::cin >> l_maxThreadCount;
	std::cout << "Minimum number of items per thread?:";
	std::cin >> l_minItemCount;
	std::cout << "Maximum number of items per thread?:";
	std::cin >> l_maxItemCount;
	std::cout << "Item count increment?:";
	std::cin >> l_itemCountIncrement;

	std::queue < std::thread> l_threadList;

	using namespace std::chrono;
	system_clock::time_point l_iCntStart;
	milliseconds l_updateDuration;

	for (int l_curItemCount = l_minItemCount; l_curItemCount <= l_maxItemCount; l_curItemCount += l_itemCountIncrement)
	{
		for (int l_curThreadCount = l_minThreadCount; l_curThreadCount <= l_maxThreadCount; ++l_curThreadCount)
		{
			g_s = new threading::lockless_queue<std::string>();
			std::atomic<bool> l_running = false;


			l_iCntStart = system_clock::now();
			l_threadList.push(std::thread(writeSomething, "This is a test string:)", l_curItemCount));
			while (l_threadList.size() > 0)
			{
				l_threadList.front().join();
				l_threadList.pop();
			}

			std::thread l_reader1(getSomething, std::ref(l_running));
			std::thread l_reader2(getSomething, std::ref(l_running));

			l_reader1.join();
			l_reader2.join();

			l_updateDuration = duration_cast<milliseconds>(system_clock::now() - l_iCntStart);

			std::cout << l_curThreadCount << " Threads " << l_curItemCount << " Items " << " Took " << l_updateDuration.count() << std::endl;
		}
	}

}
