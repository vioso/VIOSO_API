#include "../VIOSOWarpBlend/logging.cpp"
#include <iostream>
#include <chrono>
#include <random>
#include <list>
#include <numbers>
#include <cmath>
#include <limits>
#include "../VIOSOWarpBlend/mmath.h"

#include "../VIOSOWarpBlend/test_mmath.cpp"
#include "../VIOSOWarpBlend/test_threadpool.cpp"

int main( int argc, char* argv[] )
{
	using namespace std;

	g_logLevel = 5;
	logStr( 0, "-- START test --" );
	test_mmath();
	test_threadpool();
}