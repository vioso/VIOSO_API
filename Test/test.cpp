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

#include "../VIOSOWarpBlend/JSON.h"
#include "../VIOSOWarpBlend/JSONRPC.h"

int main( int argc, char* argv[] )
{
	using namespace std;

	g_logLevel = 5;
	logStr( 0, "-- START test --" );
	//test_socket();
	//test_mmath();
	//test_threadpool();

	std::u8string str = u8"süßse\tMöhre\n\t\tKäuter\f\b\u8dc0 I-Ging";
	auto encoded = JSON::encode( str );
	auto decoded = JSON::decode( encoded );
	if( decoded != str )
		logStr( 0, "encode/decode test failed" );

	auto native1 = to_string( decoded );
	auto native2 = to_string( str );

	JSON fromObject( JSON::Object( { { u8"name", u8"stringvalue" } } ) );
	auto s = fromObject.to_string();
	JSON fromString( "{ \"a\" : \"b\", \"c\" : true, \"eee\": { \"a\" : [ 5, -10,false,0.3 , \"hello\\u00F2\", null ]}, \"null\":null }" );
	u8string us = fromString[u8"a"];
	bool b = fromString[u8"c"];
	double d = fromString[u8"eee"][u8"a"][3];
	unsigned int u = fromString[u8"eee"][u8"a"][1];
	int i = fromString[u8"eee"][u8"a"][1];
	s = fromString.to_string();
	JSON newBuild;
	newBuild[u8"foo"] = u8"bar";
	newBuild[u8"other"] = JSON::List();
	newBuild[u8"other"][size_t(0)] = 42;
	newBuild[u8"other"][1] = 43;
	newBuild[u8"other"][2] = true;
	newBuild[u8"other"].push_back( JSON::Value(44) );

	s = newBuild.to_string();
	return 0;

	//struct Handler {};
	//struct Setter
	//{
	//	Handler* ref;
	//	std::function<bool( int, Handler* )> cb;
	//	Setter( std::function<bool( int, Handler* )>&& _cb, Handler* _ref ) : cb( std::move( _cb ) ), ref( _ref ) {}
	//};
	//std::map<std::string, Setter > setters;

	//setters.emplace( "name", Setter( []( int i, Handler* ref ) { return true; }, nullptr ) );

}