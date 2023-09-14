#include "../VIOSOWarpBlend/logging.cpp"
#include <iostream>
#include <chrono>
#include <random>
#include <list>
#include <numbers>
#include <cmath>
#include <limits>
#include "../VIOSOWarpBlend/mmath.h"

#include "../VIOSOWarpBlend/test_socket.cpp"
#include "../VIOSOWarpBlend/test_mmath.cpp"
#include "../VIOSOWarpBlend/test_threadpool.cpp"
#include "../VIOSOWarpBlend/JSONWrapper.h"

int main( int argc, char* argv[] )
{
	g_logLevel = 5;
	logStr( 0, "-- START test --" );
	//test_socket();
	//test_mmath();
	//test_threadpool();

	std::string s{ "{ \"a\" : \"b\", \"c\" : true, \"eee\": { \"a\" : [ 5, 10, false, \"hello\" ]}}" };
	JSONWrapper j( s );

	return 0;
}