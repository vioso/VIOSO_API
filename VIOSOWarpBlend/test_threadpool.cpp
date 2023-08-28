#include "threadpool.h"

bool test_threadpool()
{
	using namespace std;
	threadpool::init();
	std::function<int()> fn1 = bind( []( int a, int b ) { 
		return a + b; 
	}, 10, 32 );
	std::function<int()> fn2 = bind( []( int a, int b ) {
		return a * b; 
	}, 11, 22 );
	auto fut = threadpool::enqueue<int>( { fn1, fn2 }, []( int a, int b ) { 
		return a + b; 
	} );
	int c = fut.get();

	const int dx = 23840, dy = 22160, dz = 3;
	typedef float MapLT[dx][dz];
	unique_ptr<MapLT[]> map( new MapLT[dy] );

	std::chrono::high_resolution_clock cl;
	mt19937 rg;
	for( int y = 0; y != dy; y++ )
	{
		for( int x = 0; x != dx; x++ )
		{
			map[y][x][0] = float( double( rg() ) / UINT_MAX );
			map[y][x][1] = float( double( rg() ) / UINT_MAX );
			map[y][x][2] = rg() > UINT_MAX/2 ? 1.0f : 0.0f;
		}
	}
	auto start = cl.now();
	for( int y = 0; y != dy; y++ )
	{
		for( int x = 0; x != dx; x++ )
		{
			if( map[y][x][2] > 0.5f )
				swap( map[y][x], map[dy - y - 1][dx - x - 1] );
		}
	}
	auto end = cl.now();
	chrono::duration<float, milli> span = end - start;
	cout << "Single thread: " << span << endl;

	start = cl.now();
	threadpool::parallel_for_i<int>( 0, []( int y ) { return y != dy; }, []( int& y ) { y++; }, [map = map.get()]( int y )
	{
		for( int x = 0; x != dx; x++ )
		{
			if( map[y][x][2] > 0.5f )
				swap( map[y][x], map[dy - y - 1][dx - x - 1] );
		}
	} );
	end = cl.now();
	span = end - start;
	cout << "Multi thread: " << span << endl;

	return c == 284;
}
