#include "../VIOSOWarpBlend/logging.cpp"

#include "../VIOSOWarpBlend/test_socket.cpp"
#include "../VIOSOWarpBlend/test_mmath.cpp"
#include "../VIOSOWarpBlend/test_threadpool.cpp"

int main( int argc, char* argv[] )
{
	g_logLevel = 5;
	logStr( 0, "-- START test --" );
	test_socket();
	test_mmath();
	test_threadpool();
	return 0;
}