#pragma once

#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)


#include <Windows.h>
#include <stdio.h>
#include<iostream>
#include <mmsystem.h>
#include <queue>
#include <map>
#include <set>
#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <thread>
#include <unordered_set>
#include<atomic>
#include <condition_variable>
#include <iomanip>
#include<stdlib.h>
#include<conio.h>
#include<math.h>
#include <sstream>


// using chrono high_resolution_clock
using namespace std::chrono;
using hrc = high_resolution_clock;
#define ELAPSED(st, en)			( duration_cast<duration<double>>(en - st).count() )
#define ELAPSED_MS(st, en)		( duration_cast<duration<double, std::milli>>(en - st).count() )


