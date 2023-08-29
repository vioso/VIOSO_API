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

	const int dx = 23840, dy = 12160, dz = 3;
	typedef float MapLT[dx][dz];
	unique_ptr<MapLT[]> map( new MapLT[dy] );

	mt19937 rg;
	std::chrono::high_resolution_clock cl;
	for( int y = 0; y != dy; y++ )
	{
		for( int x = 0; x != dx; x++ )
		{
			map[y][x][0] = float( double( rg() ) / mt19937::_Max );
			map[y][x][1] = float( double( rg() ) / mt19937::_Max );
			map[y][x][2] = rg() > mt19937::_Max /2 ? 1.0f : 0.0f;
		}
	}
	auto start = cl.now();
	for( int y = 0; y != dy; y++ )
	{
		for( int x = 0; x != dx; x++ )
		{
			if( map[y][x][2] > 0.5f )
			{
				if( map[dy - y - 1][dx - x - 1][2] > 0.5f )
					swap( map[y][x], map[dy - y - 1][dx - x - 1] );
				else
					map[dy - y - 1][dx - x - 1][2] = 1.0f - map[dy - y - 1][dx - x - 1][2];
			}
			else
				map[y][x][2] = 1.0f - map[y][x][2];
		}
	}
	auto end = cl.now();
	chrono::duration<float, milli> span = end - start;
	cout << "Swap test pure for(;;) single thread: " << span << endl;

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
	cout << "Swap test parallel_for_i multi thread: " << span << endl;

	start = cl.now();
	threadpool::parallel_for_i_ballanced<int>( 0, []( int y ) { return y != dy; }, []( int& y ) { y++; }, [map = map.get()]( int y )
	{
		for( int x = 0; x != dx; x++ )
		{
			if( map[y][x][2] > 0.5f )
				swap( map[y][x], map[dy - y - 1][dx - x - 1] );
		}
	} );
	end = cl.now();
	span = end - start;
	cout << "Swap test parallel_for_i_ballanced multi thread: " << span << endl;

	list<VWB_MAT44d> mList1;
	list<VWB_MAT44d> mList2;
	list<VWB_MAT44d> mList3;
	for( int i = 0; i != 2000; i++ )
	{
		const double rd = 2.0 * numbers::pi / mt19937::_Max;
		double aX = rd * rg();
		double aY = rd * rg();
		double aZ = rd * rg();
		mList1.emplace_back( VWB_MAT44d::R( aX, aY, aZ ) );
		aX = rd * rg();
		aY = rd * rg();
		aZ = rd * rg();
		mList2.emplace_back( VWB_MAT44d::R( aX, aY, aZ ) );
		mList1.emplace_back( VWB_MAT44d::O() );
	}
	start = cl.now();
	for( auto const& el1 : mList1 )
	{
		for( auto const& el2 : mList2 )
		{
			mList3.emplace_back( el1 * el2 );
		}
	}
	end = cl.now();
	span = end - start;
	cout << "Matrix multiplication test pure for( : ) single thread: " << span << endl;
	mList3.clear();
	start = cl.now();
	threadpool::parallel_foreach( mList1, [&mList2,&mList3]( list<VWB_MAT44d>::iterator it)
	{
		for( auto const& el2 : mList2 )
		{
			mList3.emplace_back( (*it) * el2 );
		}
	} );
	end = cl.now();
	span = end - start;
	cout << "Matrix multiplication test parallel_foreach(  ) multi thread: " << span << endl;
	mList3.clear();
	threadpool::destroy();
	return c == 284;
}
