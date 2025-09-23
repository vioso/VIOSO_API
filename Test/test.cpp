#include "../VIOSOWarpBlend/logging.cpp"
#include <iostream>
#include <chrono>
#include <random>
#include <list>
#include <numbers>
#include <cmath>
#include <limits>
#include "../VIOSOWarpBlend/mmath.h"
#include "../Include/StringConversions.h"

void eval( int l, char const* what, bool ok ) {
	if( ok )
		logStr( l, "%s succeeded.", what );
	else
		logStr( l, "%s failed.", what );
}

#include "../VIOSOWarpBlend/test_mmath.cpp"
#include "../VIOSOWarpBlend/test_threadpool.cpp"
#include "../VIOSOWarpBlend/test_socket.cpp"
#include <codecvt>


// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template<class Facet>
struct deletable_facet : Facet
{
	template<class ...Args>
	deletable_facet( Args&& ...args ) : Facet( std::forward<Args>( args )... ) {}
	~deletable_facet() {}
};

int main( int argc, char* argv[] )
{
	using namespace std;

	g_logLevel = 5;
	logStr( 0, "-- START test --" );
	//test_mmath();
	//test_threadpool();
	test_socket();
	{
		using namespace VWBUtil;
		//set_my_locale( std::locale("en_US.UTF-8") );

		string s1 = string( "h€üüü?ßßt" ) + string( 1, '\0' ) + "tt";
		wstring ws1 = to_wstring( s1 );
		eval( 1, "string to wstring and back", s1 == to_string(ws1) );

		u8string u8s2 = u8string( u8"h\u20ACüüü?ßßt" ) + u8string( 1, '\0' ) + u8"tt";
		wstring ws2 = to_wstring( u8s2 );
		eval( 1, "u8string to wstring and back", u8s2 == to_u8string( ws2 ) );

		string s3 = to_string( u8s2 );
		eval( 1, "u8string to string and back", u8s2 == to_u8string( s3 ) );
	}
}
