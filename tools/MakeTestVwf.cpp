// Generates synthetic 2D .vwf files for tests and prints the headers of existing ones.
// It is a development tool, not a part of the API, and no sample for integrators.
// The files are written by the library's own SaveVWF(), so they follow the file format by construction.
//
//   MakeTestVwf make out.vwf [options]     (a relative path is relative to the working directory, missing folders are created)
//     --channels CxR       tiles of channels, columns x rows               (default 1x1)
//     --size WxH           pixels (= warp records) of each channel         (default 256x144)
//     --overlap N          overlap of neighbouring channels in pixels      (default 0)
//     --content WxH        size of the content space in pixels             (default: all channels, minus overlaps)
//     --content-offset X,Y position of the channels' top left corner in the
//                          content space in pixels                         (default 0,0)
//     --screen-offset X,Y  desktop position of the first channel           (default 0,0)
//     --distort A          sine distortion of the mapping, in content pixels, to see the warp (default 0)
//     --edge N             soft edge of N pixels on all outer sides of the arrangement (default 0)
//     --edge-left N, --edge-right N, --edge-top N, --edge-bottom N
//                          soft edge of one outer side of the arrangement; the sides between channels have the overlap
//     --name PREFIX        channel names are PREFIX_<column>_<row>         (default Test)
//
//   MakeTestVwf info file.vwf
//
// The mapping is linear: the warp record of a projector pixel holds the uv position of that pixel in the content
// space, normalised to the whole content space. Together with --content and --content-offset one channel
// can describe a small part of a very wide content space, e.g. to measure the precision of the warp texture
// without the memory of dozens of 4K channels.

#include "VWF.h"
#include "logging.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifdef WIN32
// normally set by the DLL's entry point, the path helper resolves relative paths against it
HINSTANCE g_hModDll = NULL;
#endif

static bool parsePair( char const* s, double& a, double& b, char sep )
{
	char* e = nullptr;
	a = strtod( s, &e );
	if( !e || sep != *e )
		return false;
	b = strtod( e + 1, &e );
	return e && 0 == *e;
}

// A relative path is relative to the working directory. The library would resolve it against the folder of the
// executable, so the path is made absolute here before it is handed over. The extension .vwf is added like the library does.
static std::string fullPath( char const* p )
{
	namespace fs = std::filesystem;
	std::error_code ec;
	fs::path f( p );
	if( !f.has_extension() )
		f.replace_extension( ".vwf" );
	fs::path a = fs::absolute( f, ec );
	return ec ? f.string() : a.lexically_normal().string();
}

static int usage( char const* reason = nullptr )
{
	if( reason )
		fprintf( stderr, "MakeTestVwf: %s\n\n", reason );
	fprintf( stderr, "usage:\n  MakeTestVwf make out.vwf [--channels CxR] [--size WxH] [--overlap N] [--content WxH]\n"
		"                           [--content-offset X,Y] [--screen-offset X,Y] [--distort A] [--name PREFIX]\n"
		"                           [--edge N] [--edge-left N] [--edge-right N] [--edge-top N] [--edge-bottom N]\n"
		"  MakeTestVwf info file.vwf\n" );
	return 2;
}

static int info( char const* arg )
{
	std::string path = fullPath( arg );
	VWB_WarpBlendHeaderSet set;
	VWB_ERROR err = ScanVWF( path.c_str(), &set );
	if( VWB_ERROR_NONE != err )
	{
		fprintf( stderr, "could not read %s (VWB_ERROR %d)\n", path.c_str(), int( err ) );
		return 1;
	}
	int i = 0;
	for( auto h : set )
	{
		auto const& hd = h->header;
		printf( "[%d] \"%s\" %dx%d flags=0x%x offset=(%.0f,%.0f) typeCalib=%.0f\n", i++, hd.name, hd.width, hd.height, hd.flags,
			hd.offsetX, hd.offsetY, hd.typeCalib );
		printf( "     vCntDispPx=[%g %g %g %g | %g %g | %g]\n", hd.vCntDispPx[0], hd.vCntDispPx[1], hd.vCntDispPx[2], hd.vCntDispPx[3],
			hd.vCntDispPx[4], hd.vCntDispPx[5], hd.vCntDispPx[6] );
		printf( "     vPartialCnt=[%g %g %g %g | aspect %g]\n", hd.vPartialCnt[0], hd.vPartialCnt[1], hd.vPartialCnt[2], hd.vPartialCnt[3],
			hd.vPartialCnt[4] );
	}
	DeleteVWF( set );
	return 0;
}

static int make( int argc, char** argv )
{
	if( argc < 1 )
		return usage( "no output file given" );
	std::string out = fullPath( argv[0] );
	double cols = 1, rows = 1, w = 256, h = 144, overlap = 0, contentW = 0, contentH = 0, cox = 0, coy = 0, sox = 0, soy = 0, distort = 0;
	std::string name = "Test";
	double edge[4] = { 0, 0, 0, 0 }; // soft edges of the outer sides in pixels: left, top, right, bottom
	for( int i = 1; i < argc; i++ )
	{
		std::string a = argv[i];
		if( i + 1 >= argc )
		{
			fprintf( stderr, "MakeTestVwf: option %s needs a value\n\n", a.c_str() );
			return usage();
		}
		char const* v = argv[++i];
		bool ok = true;
		if( "--channels" == a )
			ok = parsePair( v, cols, rows, 'x' );
		else if( "--size" == a )
			ok = parsePair( v, w, h, 'x' );
		else if( "--overlap" == a )
			overlap = atof( v );
		else if( "--content" == a )
			ok = parsePair( v, contentW, contentH, 'x' );
		else if( "--content-offset" == a )
			ok = parsePair( v, cox, coy, ',' );
		else if( "--screen-offset" == a )
			ok = parsePair( v, sox, soy, ',' );
		else if( "--distort" == a )
			distort = atof( v );
		else if( "--name" == a )
			name = v;
		else if( "--edge" == a )
			edge[0] = edge[1] = edge[2] = edge[3] = atof( v );
		else if( "--edge-left" == a )
			edge[0] = atof( v );
		else if( "--edge-top" == a )
			edge[1] = atof( v );
		else if( "--edge-right" == a )
			edge[2] = atof( v );
		else if( "--edge-bottom" == a )
			edge[3] = atof( v );
		else
			ok = false;
		if( !ok )
		{
			fprintf( stderr, "MakeTestVwf: unknown option or bad value: %s %s\n\n", a.c_str(), v );
			return usage();
		}
	}
	if( cols < 1 || rows < 1 )
		return usage( "--channels needs at least 1x1" );
	if( w < 2 || h < 2 )
		return usage( "--size needs at least 2x2" );
	if( overlap < 0 )
		return usage( "--overlap must not be negative" );
	// the overlap only exists between neighbours: it is limited by the size in that direction only
	if( cols > 1 && overlap >= w )
		return usage( "--overlap must be smaller than the width of a channel when there are several columns" );
	if( rows > 1 && overlap >= h )
		return usage( "--overlap must be smaller than the height of a channel when there are several rows" );

	for( int k = 0; k < 4; k++ )
	{
		char const* const sideName[4] = { "left", "top", "right", "bottom" };
		if( edge[k] < 0 )
			return usage( "an edge must not be negative" );
		if( edge[k] >= ( 0 == k % 2 ? w : h ) )
		{
			fprintf( stderr, "MakeTestVwf: the %s edge must be smaller than the %s of a channel\n\n", sideName[k], 0 == k % 2 ? "width" : "height" );
			return usage();
		}
	}

	double const totalW = cols * ( w - overlap ) + overlap, totalH = rows * ( h - overlap ) + overlap;
	if( contentW <= 0 || contentH <= 0 )
	{
		contentW = totalW;
		contentH = totalH;
	}
	double const pi = 3.14159265358979323846;

	VWB_WarpBlendSet set;
	for( int r = 0; r < int( rows ); r++ )
	{
		for( int c = 0; c < int( cols ); c++ )
		{
			int const W = int( w ), H = int( h );
			auto* wb = new VWB_WarpBlend{};
			auto& hd = wb->header;
			hd.flags = FLAG_WARPFILE_HEADER_OFFSET | FLAG_WARPFILE_HEADER_CALIBRATION_BASE_TYP;
			hd.width = W;
			hd.height = H;
			hd.size = W * H * sizeof( VWB_WarpRecord );
			for( int k = 0; k < 4; k++ )
				hd.white[k] = 1.0f;
			hd.black[3] = 0.0f;
			hd.typeCalib = 2; // as in the sample file
			hd.offsetX = float( sox + c * ( w - overlap ) );
			hd.offsetY = float( soy + r * ( h - overlap ) );
			snprintf( hd.name, sizeof( hd.name ), "%s_%d_%d", name.c_str(), c, r );
			strcpy( hd.hostname, "localhost" );
			hd.compoundID = 1;

			// content position of the channel's corners, normalised
			double const x0 = ( cox + c * ( w - overlap ) ) / contentW, x1 = x0 + w / contentW;
			double const y0 = ( coy + r * ( h - overlap ) ) / contentH, y1 = y0 + h / contentH;
			hd.vCntDispPx[0] = 0;
			hd.vCntDispPx[1] = 0;
			hd.vCntDispPx[2] = float( W - 1 );
			hd.vCntDispPx[3] = float( H - 1 );
			hd.vCntDispPx[4] = float( ( x1 - x0 ) / w );
			hd.vCntDispPx[5] = float( ( y1 - y0 ) / h );
			hd.vPartialCnt[0] = float( x0 );
			hd.vPartialCnt[1] = float( y0 );
			hd.vPartialCnt[2] = float( x1 );
			hd.vPartialCnt[3] = float( y1 );
			hd.vPartialCnt[4] = float( contentW / contentH );

			wb->pWarp = new VWB_WarpRecord[size_t( W ) * H];
			wb->pBlend = new VWB_BlendRecord[size_t( W ) * H];
			for( int y = 0; y < H; y++ )
			{
				for( int x = 0; x < W; x++ )
				{
					// position in the content space in pixels
					double const px = cox + c * ( w - overlap ) + x + 0.5, py = coy + r * ( h - overlap ) + y + 0.5;
					double u = px / contentW, v = py / contentH;
					if( 0 != distort )
					{
						u += distort * std::sin( 2 * pi * ( y + 0.5 ) / h ) / contentW;
						v += distort * std::sin( 2 * pi * ( x + 0.5 ) / w ) / contentH;
					}
					auto& rec = wb->pWarp[size_t( y ) * W + x];
					rec = { float( u ), float( v ), 1.0f, 0.0f };

					// linear ramps inside the overlaps, gamma is part of the blend map
					double a = 1;
					if( overlap > 0 )
					{
						if( c > 0 )
							a *= ( std::min )( 1.0, ( x + 0.5 ) / overlap );
						if( c + 1 < cols )
							a *= ( std::min )( 1.0, ( w - x - 0.5 ) / overlap );
						if( r > 0 )
							a *= ( std::min )( 1.0, ( y + 0.5 ) / overlap );
						if( r + 1 < rows )
							a *= ( std::min )( 1.0, ( h - y - 0.5 ) / overlap );
					}
					// soft edges of the outer sides of the arrangement, same ramp as the overlaps
					if( 0 == c && edge[0] > 0 )
						a *= ( std::min )( 1.0, ( x + 0.5 ) / edge[0] );
					if( 0 == r && edge[1] > 0 )
						a *= ( std::min )( 1.0, ( y + 0.5 ) / edge[1] );
					if( c + 1 == int( cols ) && edge[2] > 0 )
						a *= ( std::min )( 1.0, ( w - x - 0.5 ) / edge[2] );
					if( r + 1 == int( rows ) && edge[3] > 0 )
						a *= ( std::min )( 1.0, ( h - y - 0.5 ) / edge[3] );
					auto g = VWB_byte( std::lround( 255 * std::pow( a, 1 / 2.2 ) ) );
					wb->pBlend[size_t( y ) * W + x] = { g, g, g, 255 };
				}
			}
			set.push_back( wb );
		}
	}

	// the library does not create folders
	std::error_code ecDir;
	std::filesystem::create_directories( std::filesystem::path( out ).parent_path(), ecDir );
	VWB_ERROR err = ecDir ? VWB_ERROR_VWF_FILE_NOT_FOUND : SaveVWF( set, out.c_str() );
	for( auto wb : set )
	{
		delete[] wb->pWarp;
		delete[] wb->pBlend;
		delete wb;
	}
	if( VWB_ERROR_NONE != err )
	{
		if( ecDir )
			fprintf( stderr, "could not create the folder of %s: %s\n", out.c_str(), ecDir.message().c_str() );
		else
			fprintf( stderr, "writing %s failed (VWB_ERROR %d)\n", out.c_str(), int( err ) );
		return 1;
	}
	printf( "wrote %s: %d channel(s) of %dx%d, content space %.0fx%.0f\n", out.c_str(), int( cols * rows ), int( w ), int( h ), contentW, contentH );
	return 0;
}

int main( int argc, char** argv )
{
	g_logLevel = 0; // the library logs to stdout by default, we report ourselves
	if( argc < 3 )
		return usage();
	std::string cmd = argv[1];
	if( "make" == cmd )
		return make( argc - 2, argv + 2 );
	if( "info" == cmd )
		return info( argv[2] );
	return usage( "unknown command" );
}
