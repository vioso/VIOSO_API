// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "dpXML.h"
#include "VWF.h"
#include "mmath.h"
#include "logging.h"
#include "StringConversions.h"
#include <fstream>
#include <sstream>
#include <map>
#include <limits>
#include <span>

#define STB_IMAGE_IMPLEMENTATION
#ifdef WIN32
#define STBI_WINDOWS_UTF8
#include "stb_image.h"
#else
#include "stb/stb_image.h"
#endif // def WIN32

VWB_ERROR LoadDPFrustum( std::filesystem::path path, VWB_WarpBlend& wb, bool asTarget = false ) {
	std::ifstream ifs;
	std::string tmp;
	// open file
	ifs.open( path );
	if( !ifs.is_open() ) {
		logStr( 1, "ERROR: LoadDPFrustum: Error opening \"%s\"\n", path.string().c_str() );
		return VWB_ERROR_VWF_LOAD;
	}
	// strip BOM
	if( ifs.peek() == 0xEF ) {
		char bom[3]{};
		ifs.read( bom, 3 );
		if( !( (unsigned char)( bom[1] ) == 0xBB &&
			(unsigned char)( bom[2] ) == 0xBF ) ) {
			// No BOM, rewind
			ifs.seekg( 0 );
		}
	}
	std::getline( ifs, tmp ); // skip first (comment) line
	std::getline( ifs, tmp );
	ifs.close();
	// parse data
	auto value_views = VWBUtil::split( tmp, ';' );
	if( value_views.size() < 10 ) {
		logStr( 1, "ERROR: LoadDPFrustum malformed frustum file: too few values." );
		return VWB_ERROR_VWF_LOAD;
	}

	// orientation euler
	wb.header.dir[1] = std::stof( std::string( value_views[3] ) ); // heading is around Z axis
	wb.header.dir[0] = std::stof( std::string( value_views[4] ) ); // pitch is around X axis
	wb.header.dir[2] = std::stof( std::string( value_views[5] ) ); // bank is around Y axis

	if( asTarget ) {
		// target, we have (0)x;y;z;(3)heading;pitch;bank;(6)left;right;bottom;top;(10)width;height;(12)normalx;normaly;normalz;(15)c0x;c0y;c0z;c1x;c1y;c1z;c2x;c2y;c2z;c3x;c3y;c3z
		if( value_views.size() < 25 ) {
			wb.header.screen = 0;
			logStr( 1, "ERROR: LoadDPFrustum malformed target file: too few values." );
			return VWB_ERROR_VWF_LOAD;
		} else {
			// trans matrix: it scales Meter to Millimeter, swaps y and z and moves to pos
			auto Bi = VWB_MAT44f(
				0.001f,     0.f,    0.f, 0.f,
				0.f,     0.f, 0.001f, 0.f,
				0.f, -0.001f,    0.f, 0.f,
				0.f,     0.f,    0.f, 1.f );

			// position .pos is (0,0,0) initial and meant to move, so we don't set it here
			// the first 3 values are the perpendicular from origin to the screen plane
			auto ref = Bi * VWB_VEC3f( std::stof( std::string( value_views[0] ) ),
									   std::stof( std::string( value_views[1] ) ),
									   std::stof( std::string( value_views[2] ) ) );
			
			// screen normal
			// z is up and y is forward in domeprojection, but we need that just once here, 
			// as it will be corrected by the base transformation matrix later
			// nothing except the calculated FoVs and the screen distance
			auto normal = VWB_VEC3f( std::stof( std::string( value_views[12] ) ),
									 std::stof( std::string( value_views[14] ) ),
									-std::stof( std::string( value_views[13] ) ) );

			wb.header.screen = normal.dot( ref );

			// screen corners
			VWB_VEC3f corners[4]; // top-left, top-right, bottom-right, bottom-left
			for( int i = 0; i < 4; i++ ) {
				corners[i] = Bi * VWB_VEC3f( std::stof( std::string( value_views[15 + i * 3] ) ),
											 std::stof( std::string( value_views[16 + i * 3] ) ),
											 std::stof( std::string( value_views[17 + i * 3] ) ) );
			}

			auto dx = corners[1] + corners[2] - corners[0] - corners[3]; // left -> right
			auto dy = corners[0] + corners[1] - corners[2] - corners[3]; // down -> up
			//auto R = VWB_MAT33f::Base( dx, dy ); // right, up
			auto R = VWB_MAT33f::R( DEG2RADf( wb.header.dir[0] ), DEG2RADf( wb.header.dir[1] ), DEG2RADf( wb.header.dir[2] ) );

			float maxL = FLT_MAX, maxT = -FLT_MAX, maxR = -FLT_MAX, maxB = FLT_MAX;  // maximum horizontal and vertical view size, left top right bottom

			for( auto const& v : std::span( corners, 4 ) ) {
				auto vTT = R * v;
				float vx = -vTT.x / vTT.z;
				float vy = -vTT.y / vTT.z;

				if( maxL > vx ) // left, minimal x (x points right)
					maxL = vx;
				if( maxT < vy ) // top, maximal y (y points up)
					maxT = vy;
				if( maxR < vx ) // right, maximal x
					maxR = vx;
				if( maxB > vy ) // bottom, minimal y
					maxB = vy;
			}
			if( FLT_MAX == maxL || FLT_MAX == maxT || -FLT_MAX == maxR || -FLT_MAX == maxB ) {
				logStr( 1, "WARNING: AutoView cannot calculate FoVs.\n" );
				return VWB_ERROR_VWF_LOAD;
			}

			// calculate angles in degrees
			wb.header.fov[0] = VWB_float( RAD2DEG( atan( -maxL ) ) );   // left
			wb.header.fov[1] = VWB_float( RAD2DEG( atan( maxT ) ) );	 // top
			wb.header.fov[2] = VWB_float( RAD2DEG( atan( maxR ) ) );	 // right
			wb.header.fov[3] = VWB_float( RAD2DEG( atan( -maxB ) ) );	 // bottom
		}
	} else {
		// frustum
		// we have (0)x;y;z;(3)heading;pitch;bank;(6)left;right;bottom;top;(10)tanLeft;tanRight;tanBottom;tanTop;(14)width;height

		// position, this is the eye point in case of frustum, in case of target, this is the perpendicular 
		wb.header.pos[0] = std::stof( std::string( value_views[0] ) );
		wb.header.pos[1] = std::stof( std::string( value_views[1] ) );
		wb.header.pos[2] = std::stof( std::string( value_views[2] ) ); // TODO: check if we need axis swap

		// -left, right, -bottom, top -> left, top, right, bottom, these are angles in case of frustum, sizes of the projection plane if target
		wb.header.fov[0] = -std::stof( std::string( value_views[6] ) );
		wb.header.fov[1] = std::stof( std::string( value_views[9] ) );
		wb.header.fov[2] = std::stof( std::string( value_views[7] ) );
		wb.header.fov[3] = -std::stof( std::string( value_views[8] ) );

		wb.header.screen = 1; // same as screen z-coordinate set, when warpmap is loaded
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR LoadDPShape(std::filesystem::path path, VWB_WarpBlendMeshEx*& pMesh, bool flipVertices, bool flipUvs ){
	std::ifstream ifs;
	std::string tmp;

	// open file
	ifs.open(path);
	if (!ifs.is_open())	{
		logStr( 1, "ERROR: LoadDPShape: Error opening \"%s\"\n", path.string().c_str() );
		return VWB_ERROR_VWF_LOAD;
	}

	// strip BOM
	if( ifs.peek() == 0xEF ) {
		char bom[3]{};
		ifs.read(bom, 3);
		if( !( (unsigned char)( bom[1] ) == 0xBB &&
			(unsigned char)( bom[2] ) == 0xBF ) ) {
			// No BOM, rewind
			ifs.seekg( 0 );
		}
	}

	std::map< std::pair< unsigned int, unsigned int >, VWB_WarpBlendVertexEx> vertices;

	unsigned int dimX = 0;
	unsigned int dimY = 0;
	std::getline( ifs, tmp ); // skip first (comment) line
	bool hasNormals = false, hasTangents = false, hasBitangents = false;
	size_t nComponents = 0;
	while( std::getline( ifs, tmp ) ) {

		VWB_WarpBlendVertexEx v{};
		auto value_views = VWBUtil::split( tmp, ';' );
		if( value_views.size() < 7 ) {
			logStr( 1, "ERROR: LoadDPShape malformed shape file: too few values." );
			return VWB_ERROR_VWF_LOAD;
		}
		if( nComponents == 0 )
			nComponents = value_views.size();
		else if( nComponents != value_views.size() ) {
			logStr( 1, "ERROR: LoadDPShape malformed shape file: inconsistent number of values." );
			return VWB_ERROR_VWF_LOAD;
		}
		// position
		v.pos[0] = std::stof( std::string( value_views[0] ) );
		auto f = std::stof( std::string( value_views[1] ) );
		v.pos[1] = flipVertices ? 1.f - f : f;
		v.pos[2] = std::stof( std::string( value_views[2] ) );
		// texture coordinate
		v.uv[0] = std::stof( std::string( value_views[3] ) );
		f = std::stof( std::string( value_views[4] ) );
		v.uv[1] = flipUvs ? 1.f - f : f;

		if( value_views.size() >= 10 ) {
			// normal
			v.n[0] = std::stof( std::string( value_views[5] ) );
			v.n[1] = std::stof( std::string( value_views[6] ) );
			v.n[2] = std::stof( std::string( value_views[7] ) );
			hasNormals = true;
		}

		if( value_views.size() >= 13 ) {
			// tangent
			v.t[0] = std::stof( std::string( value_views[8] ) );
			v.t[1] = std::stof( std::string( value_views[9] ) );
			v.t[2] = std::stof( std::string( value_views[10] ) );
			hasTangents = true;
		}

		if( value_views.size() >= 16 ) {
			// bitangent
			v.b[0] = std::stof( std::string( value_views[11] ) );
			v.b[1] = std::stof( std::string( value_views[12] ) );
			v.b[2] = std::stof( std::string( value_views[13] ) );
			hasBitangents = true;
		}

		// grid position
		unsigned int c = std::stoi( std::string( value_views[ value_views.size() - 2 ] ) );
		unsigned int r = std::stoi( std::string( value_views[ value_views.size() - 1 ]  ) );

		if( c > dimX )
			dimX = c;
		if( r > dimY )
			dimY = r;
		vertices[{ c, r }] = v;
	}

	if( vertices.empty() ) {
		logStr( 1, "ERROR: LoadDPShape: No vertices found in shape file.\n" );
		return VWB_ERROR_VWF_LOAD;
	}

	if( !pMesh )
		pMesh = new VWB_WarpBlendMeshEx{};
	else {
		// clean up rest of the mesh
		pMesh->dim = VWB_size{}; // not used for shapes
		pMesh->nIdx = 0;
		if( pMesh->idx ) {
			delete[] pMesh->idx;
			pMesh->idx = nullptr;
		}
	}

	pMesh->gridDim.cx = dimX + 1;
	pMesh->gridDim.cy = dimY + 1;
	pMesh->has = VWB_WARPBLENDMESHEX_HAS_POS | VWB_WARPBLENDMESHEX_HAS_UV;
	if( hasNormals )
		pMesh->has |= VWB_WARPBLENDMESHEX_HAS_NORMALS;
	if( hasTangents )
		pMesh->has |= VWB_WARPBLENDMESHEX_HAS_TANGENTS;
	if( hasBitangents )
		pMesh->has |= VWB_WARPBLENDMESHEX_HAS_BITANGENTS;

	if( !pMesh->vtx || pMesh->nVtx != pMesh->gridDim.cx * pMesh->gridDim.cy ) {
		pMesh->nVtx = pMesh->gridDim.cx * pMesh->gridDim.cy;
		if( pMesh->vtx )
			delete[] pMesh->vtx;
		pMesh->vtx = new VWB_WarpBlendVertexEx[pMesh->nVtx];
	}
	for( unsigned int r = 0; r != pMesh->gridDim.cy; r++ ) {
		for( unsigned int c = 0; c != pMesh->gridDim.cx; c++ ) {
			if( auto it = vertices.find( { c, r } ); it != vertices.end() )
				pMesh->vtx[r * pMesh->gridDim.cx + c] = it->second;
			else
				pMesh->vtx[r * pMesh->gridDim.cx + c] = VWB_WarpBlendVertexEx{ std::numeric_limits<float>::quiet_NaN() }; // mark as invalid by setting pos x to NaN
		}
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR LoadDPWarp( std::filesystem::path path, VWB_WarpBlendMeshEx*& pMesh, bool flipVertices, bool flipUvs ) {
	std::ifstream ifs;
	std::string tmp;

	// open file
	ifs.open(path);
	if (!ifs.is_open())	{
		logStr( 1, "ERROR: LoadDPWarp: Error opening \"%s\"\n", path.string().c_str() );
		return VWB_ERROR_VWF_LOAD;
	}

	// strip BOM
	if( ifs.peek() == 0xEF ) {
		char bom[3]{};
		ifs.read(bom, 3);
		if( !( (unsigned char)( bom[1] ) == 0xBB &&
			(unsigned char)( bom[2] ) == 0xBF ) ) {
			// No BOM, rewind
			ifs.seekg( 0 );
		}
	}

	std::map< std::pair< unsigned int, unsigned int >, VWB_WarpBlendVertexEx> vertices;

	unsigned int dimX = 0;
	unsigned int dimY = 0;
	std::getline( ifs, tmp ); // skip first (comment) line
	while( std::getline( ifs, tmp ) ) {
		// x;y;u;v;column;row
		VWB_WarpBlendVertexEx v{};
		auto value_views = VWBUtil::split( tmp, ';' );
		if( value_views.size() != 6 ) {
			logStr( 1, "ERROR: LoadDPWarp: Malformed warp map file." );
			return VWB_ERROR_VWF_LOAD;
		}

		// in the x and y coordinates, we get the actual UVs, as these tell, where to sample blend and blacklevel from
		v.uv[0] = std::stof( std::string( value_views[0] ) );
		auto f = std::stof( std::string( value_views[1] ) );
		v.uv[1] = flipUvs ? 1.f - f : f;

		// the u and v coordinates are the position to sample from content, so we stretch these to a frustum( -1, 1, -1, 1 )
		v.pos[0] = 2.f * std::stof( std::string( value_views[2] ) ) - 1.f;
		f = 2.f * std::stof( std::string( value_views[3] ) ) - 1.f;
		v.pos[1] = flipVertices ? f : -f;
		v.pos[2] = 1.f; // same as screen distance is set when frustum is loaded

		unsigned int c = std::stoi( std::string( value_views[4] ) );
		unsigned int r = std::stoi( std::string( value_views[5] ) );

		if( c > dimX )
			dimX = c;
		if( r > dimY )
			dimY = r;
		vertices[{ c, r }] = v;
	}

	if( vertices.size() < 4 ) {
		logStr( 1, "ERROR: LoadDPWarp: Not enough vertices found in warp map file.\n" );
		return VWB_ERROR_VWF_LOAD;
	}

	if( !pMesh )
		pMesh = new VWB_WarpBlendMeshEx{};
	else {
		// clean up rest of the mesh
		pMesh->dim = VWB_size{}; // not used for shapes
		pMesh->nIdx = 0;
		if( pMesh->idx ) {
			delete[] pMesh->idx;
			pMesh->idx = nullptr;
		}
	}

	pMesh->gridDim.cx = dimX + 1;
	pMesh->gridDim.cy = dimY + 1;
	pMesh->has = VWB_WARPBLENDMESHEX_HAS_POS | VWB_WARPBLENDMESHEX_HAS_UV;

	if( !pMesh->vtx || pMesh->nVtx != pMesh->gridDim.cx * pMesh->gridDim.cy ) {
		pMesh->nVtx = pMesh->gridDim.cx * pMesh->gridDim.cy;
		if( pMesh->vtx )
			delete[] pMesh->vtx;
		pMesh->vtx = new VWB_WarpBlendVertexEx[pMesh->nVtx];
	}
	for( unsigned int r = 0; r != pMesh->gridDim.cy; r++ ) {
		for( unsigned int c = 0; c != pMesh->gridDim.cx; c++ ) {
			if( auto it = vertices.find( { c, r } ); it != vertices.end() )
				pMesh->vtx[r * pMesh->gridDim.cx + c] = it->second;
			else
				pMesh->vtx[r * pMesh->gridDim.cx + c] = VWB_WarpBlendVertexEx{ std::numeric_limits<float>::quiet_NaN() }; // mark as invalid by setting pos x to NaN
		}
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR LoadDPXML( VWB_WarpBlendSet& set, std::vector<std::filesystem::path> const& paths, bool flipVertices, bool flipUvs ) {
	VWB_WarpBlend wb{};
	wb.header.blackScale = 1.0f;
	wb.header.blackDark = 1.0f;
	wb.header.blackBright = 1.0f;

	if( paths.size() != 9 )
		return VWB_ERROR_PARAMETER;

	if( !paths[0].empty() ) {
		if( VWB_ERROR_NONE == LoadDPWarp( paths[0], wb.pMesh, flipVertices, flipUvs ) ) { // warp
			wb.header.flags |= FLAG_WARPFILE_HEADER_MESH;
			logStr( 2, "INFO: LoadDP: Loaded warp from \"%s\"\n", paths[0].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading warp from \"%s\"\n", paths[0].string().c_str() );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[1].empty() ) { // blend
		int w = 0, h = 0;
		//if( stbi_is_16_bit( (char const*)paths[1].u8string().c_str() ) ) {
		//	auto blendData = stbi_load_16( (char const*)paths[1].u8string().c_str(), &w, &h, NULL, 4 );
		//	if( blendData ) {
		//		wb.header.width = w;
		//		wb.header.height = h;
		//		wb.pBlend2 = new VWB_BlendRecord2[w * h];
		//		wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
		//		wb.header.flags &= ~FLAG_WARPFILE_HEADER_BLENDV2;
		//		memcpy( wb.pBlend, blendData, w * h * 4 );
		//		stbi_image_free( blendData );
		//		logStr( 2, "INFO: LoadDP: 16-bit Blend file \"%s\" loaded.\n", paths[1].string().c_str() );
		//	} else {
		//		logStr( 1, "ERROR: LoadDP: Error loading 16-bit blend from \"%s\"\n", paths[1].string().c_str() );
		//		DeleteVWF( wb );
		//		return VWB_ERROR_VWF_LOAD;
		//	}
		//} else {
			auto blendData = stbi_load( (char const*)paths[1].u8string().c_str(), &w, &h, NULL, 4 );
			if( blendData ) {
				wb.header.width = w;
				wb.header.height = h;
				wb.pBlend = new VWB_BlendRecord[w * h];
				wb.header.flags &= ~( FLAG_WARPFILE_HEADER_BLENDV2 | FLAG_WARPFILE_HEADER_BLENDV3 );
				memcpy( wb.pBlend, blendData, w * h * 4 );
				stbi_image_free( blendData );
				logStr( 2, "INFO: LoadDP: 8-bit Blend file \"%s\" loaded.\n", paths[1].string().c_str() );
			} else {
				logStr( 1, "ERROR: LoadDP: Error loading 8-bit blend from \"%s\"\n", paths[1].string().c_str() );
				DeleteVWF( wb );
				return VWB_ERROR_VWF_LOAD;
			}
		//}
	} 

	if( !paths[2].empty() ) { // blacklevel
		int w = 0, h = 0;
		auto blackData = stbi_load( (char const*)paths[2].u8string().c_str(), &w, &h, NULL, 4 );
		if( blackData ) {
			if( 0 == wb.header.width ) {
				wb.header.width = w;
				wb.header.height = h;
			} else if( wb.header.width != w || wb.header.height != h ) {
				stbi_image_free( blackData );
				DeleteVWF( wb );
				logStr( 1, "ERROR: LoadDP: Blacklevel image size does not match blend size \"%s\"\n", paths[2].string().c_str() );
				return VWB_ERROR( VWB_ERROR_PARAMETER );
			}
			wb.pBlack = new VWB_BlendRecord[w * h];
			memcpy( wb.pBlack, blackData, w * h * 4 );
			wb.header.flags |= FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR;
			stbi_image_free( blackData );
			logStr( 2, "INFO: LoadDP: Blacklevel file \"%s\" loaded.\n", paths[2].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading blacklevel from \"%s\"\n", paths[2].string().c_str() );
			DeleteVWF( wb );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[3].empty() ) { // whitelevel
		int w = 0, h = 0;
		auto whiteData = stbi_load( (char const*)paths[3].u8string().c_str(), &w, &h, NULL, 4 );
		if( whiteData ) {
			if( 0 == wb.header.width ) {
				wb.header.width = w;
				wb.header.height = h;
			} else if( wb.header.width != w || wb.header.height != h ) {
				stbi_image_free( whiteData );
				DeleteVWF( wb );
				logStr( 1, "ERROR: LoadDP: Whitelevel image size does not match blend size \"%s\"\n", paths[3].string().c_str() );
				return VWB_ERROR( VWB_ERROR_PARAMETER );
			}
			wb.pWhite = new VWB_BlendRecord[w * h];
			memcpy( wb.pWhite, whiteData, w * h * 4 );
			wb.header.flags |= FLAG_WARPFILE_HEADER_WHITELEVEL_CORR;
			stbi_image_free( whiteData );
			logStr( 2, "INFO: LoadDP: Whitelevel file \"%s\" loaded.\n", paths[3].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading whitelevel from \"%s\"\n", paths[3].string().c_str() );
			DeleteVWF( wb );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[4].empty() ) { // 2nd blend
		int w = 0, h = 0;
		auto blend2Data = stbi_load( (char const*)paths[2].u8string().c_str(), &w, &h, NULL, 4 );
		if( blend2Data ) {
			if( 0 == wb.header.width ) {
				wb.header.width = w;
				wb.header.height = h;
			} else if( wb.header.width != w || wb.header.height != h ) {
				stbi_image_free( blend2Data );
				DeleteVWF( wb );
				logStr( 1, "ERROR: LoadDP: 2nd Blend image size does not match blend size \"%s\"\n", paths[3].string().c_str() );
				return VWB_ERROR( VWB_ERROR_PARAMETER );
			}
			wb.p2ndBlend = new VWB_BlendRecord[w * h];
			memcpy( wb.p2ndBlend, blend2Data, w * h * 4 );
			wb.header.flags |= FLAG_WARPFILE_HEADER_2NDBLEND;
			stbi_image_free( blend2Data );
			logStr( 2, "INFO: LoadDP: 2nd Blend file \"%s\" loaded.\n", paths[3].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading 2nd blend from \"%s\"\n", paths[3].string().c_str() );
			DeleteVWF( wb );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[5].empty() ) {
		if( VWB_ERROR_NONE == LoadDPShape( paths[5], wb.pMesh, flipVertices, flipUvs ) ) { // shape
			wb.header.flags |= FLAG_WARPFILE_HEADER_MESH;
			wb.header.flags |= FLAG_WARPFILE_HEADER_3D;
			logStr( 2, "INFO: LoadDP: Shape file \"%s\" loaded.\n", paths[5].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading warp from \"%s\"\n", paths[5].string().c_str() );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[6].empty() ) {
		if( VWB_ERROR_NONE == LoadDPFrustum( paths[6], wb ) ) { // frustum
			logStr( 2, "INFO: LoadDP: Frustum file \"%s\" loaded.\n", paths[6].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading warp from \"%s\"\n", paths[6].string().c_str() );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[7].empty() ) {
		if( VWB_ERROR_NONE == LoadDPFrustum( paths[7], wb, true ) ) { // target
			logStr( 2, "INFO: LoadDP: Target file \"%s\" loaded.\n", paths[7].string().c_str() );
		} else {
			logStr( 1, "ERROR: LoadDP: Error loading warp from \"%s\"\n", paths[7].string().c_str() );
			return VWB_ERROR_VWF_LOAD;
		}
	}

	if( !paths[6].empty() && !paths[7].empty() ) {
		logStr( 1, "WARNING: LoadDP: Both frustum and target files specified, using target.\n" );
	}

	if( paths[6].empty() && paths[7].empty() ) {
		logStr( 1, "WARNING: LoadDP: no frustum or target file specified, enabling autoView.\n" );
		wb.header.screen = FLT_MAX; // mark for autoView
	}

	if( !paths[8].empty() ) { // directional-shading
		if( wb.pMesh && ( VWB_WARPBLENDMESHEX_HAS_NORMALS | VWB_WARPBLENDMESHEX_HAS_TANGENTS ) != ( ( VWB_WARPBLENDMESHEX_HAS_NORMALS | VWB_WARPBLENDMESHEX_HAS_TANGENTS ) & wb.pMesh->has ) ) {
			logStr( 1, "WARNING: No normals and tangents in shape. Directional shading disabled." );
		} else {
			int w = 0, h = 0;
			auto dirShadeData = stbi_load( (char const*)paths[8].u8string().c_str(), &w, &h, NULL, 4 );
			if( dirShadeData ) {
				wb.pDirectional = new VWB_BlendRecord[w * h];
				memcpy( wb.pDirectional, dirShadeData, w * h * 4 );
				wb.header.flags |= FLAG_WARPFILE_HEADER_DIRSHADING;
				wb.directionalSz.cx = w;
				wb.directionalSz.cy = h;
				stbi_image_free( dirShadeData );
				logStr( 2, "INFO: LoadDP: Directional-Shading file \"%s\" loaded.\n", paths[3].string().c_str() );
			} else {
				logStr( 1, "ERROR: LoadDP: Error loading Directional-Shading from \"%s\"\n", paths[8].string().c_str() );
				DeleteVWF( wb );
				return VWB_ERROR_VWF_LOAD;
			}
		}
	}

	if( 0 == wb.header.width || 0 == wb.header.height ) {
		logStr( 1, "ERROR: LoadDP: No valid blend or black/whitelevel image loaded.\n" );
		DeleteVWF( wb );
		return VWB_ERROR_VWF_LOAD;
	}

	if( !wb.pMesh ) {
		logStr( 1, "ERROR: LoadDP: No valid warp or shape loaded.\n" );
		DeleteVWF( wb );
		return VWB_ERROR_VWF_LOAD;
	}

	// triangulate
	wb.pMesh->idx = new VWB_uint[ ( wb.pMesh->gridDim.cx - 1 ) * ( wb.pMesh->gridDim.cy - 1 ) * 6 ];
	auto ii = wb.pMesh->idx;
	for( unsigned int y = 1; y < wb.pMesh->gridDim.cy; y++ )
	{
		for( unsigned int x = 1; x < wb.pMesh->gridDim.cx; x++ )
		{
			// case A       B
			//    a----b  a----b
			//    | 1 /|  |\  4|
			//    |  / |  | \  |
			//    | / 2|  |3 \ |
			//    c----d  c----d

			VWB_int a = ( y - 1 ) * wb.pMesh->gridDim.cx + x - 1;
			VWB_int b = ( y - 1 ) * wb.pMesh->gridDim.cx + x;
			VWB_int c = ( y ) * wb.pMesh->gridDim.cx + x - 1;
			VWB_int d = ( y ) * wb.pMesh->gridDim.cx + x;

			// we skip triangles with NaN values
			if( !isnan( wb.pMesh->vtx[a].pos[0] ) ) {
				if( !isnan( wb.pMesh->vtx[b].pos[0] ) ) {
					if( !isnan( wb.pMesh->vtx[c].pos[0] ) ) { // we have a,b,c
						// first triangle is 1
						*ii = a; ii++;
						*ii = b; ii++;
						*ii = c; ii++;
						if( !isnan( wb.pMesh->vtx[d].pos[0] ) ) { // and also d
							// second triangle is 2
							*ii = b; ii++;
							*ii = d; ii++;
							*ii = c; ii++;
						} else {
							// no second triangle
							int i = 0; // keep to set a breakpoint here
						}
					} else if( !isnan( wb.pMesh->vtx[d].pos[0] ) ) { // we have a,b,d
						// only triangle is 4
						*ii = a; ii++;
						*ii = b; ii++;
						*ii = d; ii++;
					} else {
						// all vertices are NaN, skip
						int i = 0; // keep to set a breakpoint here
					}
				} else if( !isnan( wb.pMesh->vtx[c].pos[0] ) && !isnan( wb.pMesh->vtx[d].pos[0] ) ) { // we have a,c,d
					// only triangle is 3
					*ii = a; ii++;
					*ii = d; ii++;
					*ii = c; ii++;
				} else {
					// all vertices are NaN, skip
					int i = 0; // keep to set a breakpoint here
				}
			} else if( !isnan( wb.pMesh->vtx[b].pos[0] ) && !isnan( wb.pMesh->vtx[d].pos[0] ) && !isnan( wb.pMesh->vtx[c].pos[0] ) ) { // we have b,c,d
				// only triangle is 2
				*ii = b; ii++;
				*ii = d; ii++;
				*ii = c; ii++;
			} else {
				// all vertices are NaN, skip
				int i = 0; // keep to set a breakpoint here
			}
		}
	}
	wb.pMesh->nIdx = (VWB_uint)( ii - wb.pMesh->idx );

	set.push_back( new VWB_WarpBlend( wb ) );
	return VWB_ERROR_NONE;
}
