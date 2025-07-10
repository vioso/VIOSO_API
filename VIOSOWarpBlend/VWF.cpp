// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "Platform.h"

#include "VWF.h"
#include "logging.h"
#include "PathHelper.h"
#include "3rdparty/aes/aes.hpp"

#include <stdlib.h>
#include <fstream>
#include <math.h>

const uint8_t _iv[]{ 0xd7, 0x04, 0xa7, 0x9f, 0xad, 0x4a, 0x48, 0xb6, 0x1f, 0x3e, 0x55, 0x30, 0x46, 0x6a, 0xa1, 0x2d };

bool DeleteVWF( VWB_WarpBlend& wb )
{
		if( wb.pWarp ) 
			delete[] wb.pWarp;
		if( wb.pBlend )
			delete[] wb.pBlend;
		if( wb.pBlack ) 
			delete[] wb.pBlack;
		if( wb.pWhite ) 
			delete[] wb.pWhite;
		wb.header.width = 0;
		wb.header.height = 0;
	return true;
}

bool DeleteVWF( VWB_WarpBlendSet& set )
{
	for( VWB_WarpBlendSet::iterator it = set.begin(); it != set.end(); it = set.erase( it ) )
	{
		if( NULL != *it )
		{
			DeleteVWF(**it);
			delete (*it);
		}
	}
	return true;
}

bool DeleteVWF( VWB_WarpBlendHeaderSet& set )
{
	for( VWB_WarpBlendHeaderSet::iterator it = set.begin(); it != set.end(); it = set.erase( it ) )
	{
		if( NULL != *it )
		{
			delete (*it);
		}
	}
	return true;
}

// make sure to free() pBmi and delete[] pData after use!
VWB_ERROR LoadBMP( std::istream& is, BITMAPFILEHEADER& bmfh, BITMAPINFO*& pBmi, char*& pData, bool bSkipData )
{
	if( is.bad() )
		return VWB_ERROR_PARAMETER;

	if( !is.read( (char*)&bmfh, sizeof( BITMAPFILEHEADER ) ).eof() )
	{
		// read header
		BITMAPINFOHEADER bmih = { 0 };
		VWB_uint colorTableLength = 0;
		if( !is.read( (char*)&bmih, sizeof( bmih ) ).eof() )
		{
			switch( bmih.biBitCount ) {
			case 24:
				colorTableLength = bmih.biClrUsed;
				break;
			case 16:
			case 32:
				if( bmih.biCompression == BI_RGB )
					colorTableLength = bmih.biClrUsed;
				else if( bmih.biCompression == BI_BITFIELDS )
					colorTableLength = 3;
				break;
			default: // for 0, 1, 2, 4, and 8
				if( bmih.biClrUsed == 0 )
					colorTableLength = ( 1 << bmih.biBitCount ); // 2^biBitCount
				else
					colorTableLength = bmih.biClrUsed;
				break;
			}
			pBmi = (BITMAPINFO*)malloc( sizeof( BITMAPINFOHEADER ) + colorTableLength * sizeof( RGBQUAD ) );
			memcpy( pBmi, &bmih, sizeof( BITMAPINFOHEADER ) );
			if( 0 != colorTableLength )
				is.read( (char*)pBmi->bmiColors, colorTableLength * sizeof( RGBQUAD ) );
		}
		// skip to data
		is.seekg( bmfh.bfOffBits - sizeof( BITMAPFILEHEADER ) - sizeof( BITMAPINFOHEADER ) - colorTableLength * sizeof( RGBQUAD ), std::ios_base::cur );
		if( 0 == pBmi->bmiHeader.biSizeImage )
		{
			int stride = ( ( ( ( bmih.biWidth * bmih.biBitCount ) + 31 ) & ~31 ) >> 3 );
			pBmi->bmiHeader.biSizeImage = stride * abs( bmih.biHeight );
		}
		if( 0 == pBmi->bmiHeader.biSizeImage || is.bad() )
		{
			free( pBmi );
			pBmi = nullptr;
			return VWB_ERROR_VWF_LOAD;
		}
		if( bSkipData )
		{
			pData = nullptr;
			is.seekg( pBmi->bmiHeader.biSizeImage, std::ios_base::cur );
		}
		else
		{
			pData = new char[pBmi->bmiHeader.biSizeImage];
			is.read( pData, pBmi->bmiHeader.biSizeImage );
		}
		return is.eof() ? VWB_ERROR_VWF_LOAD : VWB_ERROR_NONE;
	}
	return VWB_ERROR_VWF_LOAD;
}

VWB_ERROR convertFromBitmapData( VWB_BlendRecord*& pB, char* pBMData, int w, int h, int depth, bool bTopDown )
{
	if( pBMData && 
		8 <= w && 
		8 <= h && (
			128 == depth ||
			96 == depth ||
			64 == depth ||
			48 == depth ||
			32 == depth ||
			24 == depth )
		)
	{
		int n = w * h;
		int bmStep = depth / 8;
		int bmPitch = ( ( ( ( w * depth ) + 31 ) & ~31 ) >> 3 );
		int bmPadding = bmPitch - w * bmStep;
		//int bmSize = bmPitch * h;
		VWB_BlendRecord2*& pB2 = *(VWB_BlendRecord2**)&pB;
		VWB_BlendRecord3*& pB3 = *(VWB_BlendRecord3**)&pB;
		if( 64 < depth )
		{
			pB3 = new VWB_BlendRecord3[n];

		}
		if( 32 < depth )
		{
			pB2 = new VWB_BlendRecord2[n];
		}
		else
		{
			pB = new VWB_BlendRecord[n];
		}
		if( bTopDown )
		{
			// trivial, just copy the whole buffer
			if( 128 == depth )
				memcpy( pB3, pBMData, n * sizeof( VWB_BlendRecord3 ) ); 
			else if( 64 == depth )
				memcpy( pB2, pBMData, n * sizeof( VWB_BlendRecord2 ) );
			else if( 32 == depth )
				memcpy( pB, pBMData, n * sizeof( VWB_BlendRecord ) );
			else if( 96 == depth )
			{
				char const* pBML = pBMData;
				for( VWB_BlendRecord3* pL = pB3, *pLE = pB3 + n; pL != pLE; pBML += bmPadding )
				{
					for( VWB_BlendRecord3* pLLE = pL + w; pL != pLLE; pL++, pBML += 12 )
					{
						VWB_float* pF = (VWB_float*)pBML;
						pL->r = pF[2];
						pL->g = pF[1];
						pL->b = pF[0];
						pL->a = 1.0f;
					}
				}
			}
			else if( 48 == depth )
			{
				char const* pBML = pBMData;
				for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pBML += bmPadding )
				{
					for( VWB_BlendRecord2* pLLE = pL + w; pL != pLLE; pL++, pBML += 6 )
					{
						VWB_word* pW = (VWB_word*)pBML;
						pL->r = pW[2];
						pL->g = pW[1];
						pL->b = pW[0];
						pL->a = 65535;
					}
				}
			}
			else if( 24 == depth )
			{
				char const* pBML = pBMData;
				for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pBML += bmPadding )
					for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML += 3 )
					{
						pL->r = pBML[2];
						pL->g = pBML[1];
						pL->b = pBML[0];
						pL->a = 255;
					}
			}
		}
		else
		{
			// reverse line order
			if( 128 == depth )
			{
				char const* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord3* pL = pB3, *pLE = pB3 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch ) // padding is always 0
					memcpy( pL, pBML, w * sizeof( VWB_BlendRecord3 ) );
			}
			else if( 64 == depth )
			{ 
				char const* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch ) // padding is always 0
					memcpy( pL, pBML, w * sizeof( VWB_BlendRecord2 ) );
			}
			else if( 96 == depth )
			{
				char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord3* pL = pB3, *pLE = pB3 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch - (ptrdiff_t)bmPadding )
				{
					for( VWB_BlendRecord3* pLLE = pL + w; pL != pLLE; pL++, pBML += 12 )
					{ // todo: interpret bitfield mask here
						VWB_float* pF = (VWB_float*)pBML;
						pL->r = pF[2];
						pL->g = pF[1];
						pL->b = pF[0];
						pL->a = 1.0f;
					}
				}
			}
			else if( 48 == depth )
			{
				char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch - (ptrdiff_t)bmPadding )
				{
					for( VWB_BlendRecord2* pLLE = pL + w; pL != pLLE; pL++, pBML += 6 )
					{ // todo: interpret bitfield mask here
						VWB_word* pW = (VWB_word*)pBML;
						pL->r = pW[2];
						pL->g = pW[1];
						pL->b = pW[0];
						pL->a = 65535;
					}
				}
			}
			else if( 32 == depth )
			{ // reverse order
				char const* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch ) // padding is always 0
					memcpy( pL, pBML, n * sizeof( VWB_BlendRecord ) );
			}
			else
			{
				char const* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
				for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch - (ptrdiff_t)bmPadding )
				{
					for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML += 3 )
					{ // todo: interpret bitfield mask here
						pL->r = pBML[2];
						pL->g = pBML[1];
						pL->b = pBML[0];
						pL->a = 255;
					}
				}
			}
		}
		return VWB_ERROR_NONE;
	}
	return VWB_ERROR_PARAMETER;
}

template< class T >
VWB_ERROR LoadHeader( T& header, std::istream& is, size_t sz )
{
	if( is.bad() )
		return VWB_ERROR_PARAMETER;

	// if we bound to load a newer (bigger) version of a header, just load the known part
	size_t padd = 0;
	if(sz > sizeof(T))
	{
		padd = sz - sizeof(T);
		sz = sizeof(T);
	}

	is.read( (char*)&header, sz );
	is.seekg( padd, std::ios_base::cur );

	if( !is.eof() )
	{
		header.szHdr = sizeof( T );
		return VWB_ERROR_NONE;
	}
	return VWB_ERROR_VWF_LOAD;
}

VWB_ERROR LoadWarp( VWB_WarpRecord*& wr, std::istream& is, size_t nRecords )
{
	if( is.bad() )
		return VWB_ERROR_PARAMETER;
	wr = new VWB_WarpRecord[nRecords];
	return is.read( (char*)wr, nRecords * sizeof( VWB_WarpRecord ) ).eof() ? VWB_ERROR_VWF_LOAD : VWB_ERROR_NONE;
}

VWB_ERROR LoadVWF( VWB_WarpBlendSet& set, char const* path, bool bScanOnly, int iScanIndex, const uint8_t* aesKey )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: LoadVWF: Missing path to warp file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	VWB_ERROR ret = VWB_ERROR_NONE;
	char const* pP = path;
	char pp[MAX_PATH];
	char const* pP2;
	int iExtMap = 0;
	do
	{
		VWB_WarpBlendSet::size_type nBefore = set.size();
		pP2 = strchr( pP, ',' );
		if( pP2 )
			strncpy_s( pp, pP, pP2 - pP );
		else
			strcpy_s( pp, pP );
		MkPath( pp, MAX_PATH, ".vwf" );

		logStr( 2, "Open \"%s\"...\n", pp );
		std::ifstream ifs( pp, std::ios_base::in | std::ios_base::binary );
		if( !ifs.fail() )
		{
			logStr( 2, "File found and openend.\n" );
			VWB_WarpSetFileHeader h0;
			int nSets = 1;
			while( nSets )
			{
				std::streampos s = ifs.tellg();
				if( !ifs.read( (char*)&h0, sizeof( VWB_WarpSetFileHeader ) ).eof() )
				{
					switch( *(VWB_uint*)&h0.magicNumber )
					{
					case '3fwv':
					case '2fwv':
					case '0fwv':
						{
							logStr( 2, "Load warp map %d...\n", VWB_uint(set.size()) );
							ifs.seekg( -(std::streampos)sizeof( VWB_WarpSetFileHeader ), std::ios_base::cur );
							VWB_WarpBlend* pWB = new VWB_WarpBlend;
							memset( pWB, 0, sizeof( VWB_WarpBlend ) );

							if( VWB_ERROR_NONE == LoadHeader( pWB->header, ifs, h0.numBlocks ) )
							{
								int nRecords = pWB->header.width * pWB->header.height;

								if( 0 != nRecords )
								{
									// make up some fake monitor handle
									if( 0 == pWB->header.hMonitor )
										pWB->header.hMonitor = 1 + VWB_uint( VWB_word( pWB->header.offsetX ) ) + ( VWB_uint( VWB_word( pWB->header.offsetY ) ) << 16 );

									strcpy_s( pWB->path, pp );

									if( 0 == pWB->header.size )
									{
										set.push_back( pWB );
										iExtMap = 0;
										nSets--;
										logStr( 2, "Empty warp map successfully added, new dataset (%d) %dx%d created.\n", VWB_uint( set.size() ), pWB->header.width, pWB->header.height );
										break;
									}
									else if( nRecords * sizeof( VWB_WarpRecord ) == pWB->header.size )
									{
										if( bScanOnly || ( -1 != iScanIndex && set.size() != iScanIndex ) )
										{
											// skip data
											set.push_back( pWB );
											ifs.seekg( pWB->header.size, std::ios_base::cur );
											iExtMap = 0;
											logStr( 2, "Warp map successfully added, new dataset (%d) %dx%d created, data loading skipped.\n", VWB_uint( set.size() ), pWB->header.width, pWB->header.height );
											nSets--;
											break;
										}
										else if( VWB_ERROR_NONE == LoadWarp( pWB->pWarp, ifs, nRecords ) )
										{
											set.push_back( pWB );
											iExtMap = 0;
											logStr( 2, "Warp map successfully loaded, new dataset (%d) %dx%d created.\n", VWB_uint( set.size() ), pWB->header.width, pWB->header.height );
											if( pWB->header.flags & FLAG_WARPFILE_HEADER_ENCRYPTED )
											{
												if( aesKey && aesKey[0] )
												{
													// test passkey
													AES_ctx ctx;
													AES_init_ctx_iv( &ctx, (uint8_t*)aesKey, _iv );
													if( !strcmp( "AES128", pWB->header.keyIdent ) )
													{
														AES_CBC_decrypt_buffer( &ctx, (uint8_t*)pWB->header.keyIdent, sizeof( pWB->header.keyIdent ) );
															if( !strcmp( "AES128", pWB->header.keyIdent ) )
															{
																logStr( 1, "WARNING: Probably wrong passkey. %s.\n", pWB->header.keyDesc );
															}
													}
													AES_CBC_decrypt_buffer( &ctx, (uint8_t*)pWB->pWarp, nRecords * sizeof( VWB_WarpRecord ) );

														nSets--;
														break;
												}
												else
													logStr( 0, "ERROR: Passkey missing. The file has been encrypted.\n" );
											}
											else
											{
												nSets--;
												break;
											}
										}
										else
											logStr( 0, "ERROR: Unexpected end of file.\n" );
									}
									else
										logStr( 0, "ERROR: Malformed file data. Data size does not match layout.\n" );
								}
								else
									logStr( 0, "ERROR: Malformed file header.\n" );
							}
							else
								logStr( 0, "ERROR: Unexpected end of file.\n" );
							ret = VWB_ERROR_VWF_LOAD;
							nSets = 0;
							delete pWB;
							break;
						}
						break;
					case '1fwv': 
						// load only file header, which is already done
						nSets = h0.numBlocks;
						ifs.seekg( h0.offs - sizeof( VWB_WarpSetFileHeader ), std::ios_base::cur ); // jump to begin of data
						logStr( 2, "File contains %d chunks.\n", nSets );
						break;
					default:
						ifs.seekg( -(std::streampos)sizeof( VWB_WarpSetFileHeader ), std::ios_base::cur );
						if( 'B' == h0.magicNumber[0] && 'M' == h0.magicNumber[1] )
						{ // load bitmap
							logStr( 2, "Load bitmap for dataset %d...\n", VWB_uint( set.size() ) );
							BITMAPFILEHEADER bmfh = { 0 };
							BITMAPINFO* pBmi = nullptr;
							char* pData = nullptr;
							if( 0 == set.size() && 0 == iExtMap ) // just a blend file
							{
								VWB_WarpBlend* pWB = new VWB_WarpBlend;
								memset( pWB, 0, sizeof( VWB_WarpBlend ) );
								pWB->header.magicNumber[0] = 'v';
								pWB->header.magicNumber[1] = 'w';
								pWB->header.magicNumber[2] = 'f';
								pWB->header.magicNumber[3] = '0';
								pWB->header.szHdr = sizeof( VWB_WarpFileHeader5 );
								set.push_back( pWB );
							}
							if( 0 != set.size() )
							{
								bool bSkipData = bScanOnly || ( -1 != iScanIndex && (set.size()-1) != iScanIndex );
								if( VWB_ERROR_NONE == LoadBMP( ifs, bmfh, pBmi, pData, bSkipData ) )
								{
									if( 0 == set.back()->header.width )
										set.back()->header.width = pBmi->bmiHeader.biWidth;
									if( 0 == set.back()->header.height )
										set.back()->header.height = abs( pBmi->bmiHeader.biHeight );
									if( pBmi->bmiHeader.biWidth == 0 || pBmi->bmiHeader.biHeight == 0 || bSkipData )  // image contains no data or must be skipped
									{
										iExtMap++;
										nSets--;
										free( pBmi );
										break;
									}
									else if( abs(pBmi->bmiHeader.biHeight) == (VWB_int)set.back()->header.height &&  // TODO optioal half sized image for future
										pBmi->bmiHeader.biWidth == (VWB_int)set.back()->header.width )
									{
										VWB_BlendRecord* p;
										if( VWB_ERROR_NONE == convertFromBitmapData( p, pData, pBmi->bmiHeader.biWidth, abs( pBmi->bmiHeader.biHeight ), pBmi->bmiHeader.biBitCount, 0 > pBmi->bmiHeader.biHeight ) )
										{
											if( 0 == iExtMap )
											{
												set.back()->header.flags &= ~( FLAG_WARPFILE_HEADER_BLENDV2 | FLAG_WARPFILE_HEADER_BLENDV3 );
												switch( pBmi->bmiHeader.biBitCount )
												{
												case 48:
												case 64:
													set.back()->header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
													break;
												case 96:
												case 128:
													set.back()->header.flags |= FLAG_WARPFILE_HEADER_BLENDV3;
													break;
												}
											}
											switch( iExtMap )
											{
											case 0:
												set.back()->pBlend = p;
												logStr( 2, "Blend image set.\n" );
												break;
											case 1:
												set.back()->pBlack = p;
												logStr( 2, "Black image set.\n" );
												break;
											case 2:
												set.back()->pWhite = p;
												logStr( 2, "White image set.\n" );
												break;
											default:
												logStr( 0, "WARNING: Too many image maps. Ignoring!\n" );
												delete[] p;
											}
											iExtMap++;
											nSets--;
											delete[] pData;
											free( pBmi );
											break;
										}
										else
										{
											logStr( 0, "ERROR: Unexpected bitmap format, must be BI_RGB with 24 bits or more. Bitmap or vwf file broken!\n" );
										}
									}
									else
									{
										logStr( 0, "ERROR: Blend map size mismatch: is %dx%d should be.\n", pBmi->bmiHeader.biWidth, abs(pBmi->bmiHeader.biWidth), set.back()->header.width, set.back()->header.height );
									}
									if( pData )
										delete[] pData;
									free( pBmi );
								}
								else
								{
									logStr( 0, "ERROR: BMP load read error.\n" );
								}
							}
							else
							{
								logStr( 0, "ERROR: Unexpected end of file. Bitmap or vwf file broken!\n" );
							}
						}
						else
							logStr( 0, "ERROR: Unknown file type \"%2c\" at %s(%u). Operation canceled.\n", h0.magicNumber, pp, (ptrdiff_t)ifs.tellg() - sizeof( VWB_WarpSetFileHeader ) );
						ret = VWB_ERROR_VWF_LOAD;
						nSets = 0;
						break;
					}
				}
				else
					ret = VWB_ERROR_VWF_LOAD;
			}
			
			logStr( 1, "LoadVWF: Successfully added %u datasets to the mappings set.\n", VWB_uint(set.size()-nBefore) );
		}
		else
		{
			logStr( 0, "ERROR: LoadVWF: Error opening \"%s\". Missing file?\n", pp );
			ret = VWB_ERROR_VWF_FILE_NOT_FOUND;
			break;
		}
		if( pP2 )
			pP = pP2 + 1;

	}while( pP2 );

	return ret;
}

template<typename T> inline void splitMap(T*& in, size_t wIn, size_t hIn, size_t oxIn, size_t oyIn, size_t wOut, size_t hOut )
{
    //UNREFERENCED_PARAMETER( hIn );
	size_t nOut = wOut * hOut;
	auto out = new T[nOut];
	const size_t padd = wIn - wOut;
	for (auto pIn = in + (oxIn + oyIn * wIn), pOut = out, pOutE = out + nOut; pOut != pOutE; pIn += padd)
	{
		for (auto pOutLE = pOut + wOut; pOut != pOutLE; pOut++, pIn++)
		{
			*pOut = *pIn;
		}
	}
	delete[] in;
	in = out;
}

VWB_ERROR SplitVWF(VWB_WarpBlend& wb, const VWB_word(&calibSplit)[4])
{
	if (0 == calibSplit[0] || 0 == calibSplit[1])
		return VWB_ERROR_FALSE;
	if (1 == calibSplit[0] && 1 == calibSplit[1])
		return VWB_ERROR_FALSE;
	if( 0 != wb.header.width % calibSplit[0] || 0 != wb.header.height % calibSplit[1] )
		return VWB_ERROR_PARAMETER;
	if (calibSplit[0] <= calibSplit[2] || calibSplit[1] <= calibSplit[3])
		return VWB_ERROR_PARAMETER;
	
	int newWidth = wb.header.width / calibSplit[0];
	int newHeight = wb.header.height / calibSplit[1];
	int offsX = newWidth * calibSplit[2];
	int offsY = newHeight * calibSplit[3];

	if (wb.pWarp)
	{
		splitMap(wb.pWarp, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
	}
	if (wb.pBlend)
	{
		if (wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3)
		{
			splitMap(wb.pBlend3, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
		}
		else if (wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2)
		{
			splitMap(wb.pBlend2, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
		}
		else
		{
			splitMap(wb.pBlend, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
		}
	}
	if (wb.pBlack)
	{
		splitMap(wb.pBlack, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
	}
	if (wb.pWhite)
	{
		splitMap(wb.pWhite, wb.header.width, wb.header.height, offsX, offsY, newWidth, newHeight);
	}

	wb.header.width = newWidth;
	wb.header.height = newHeight;
	wb.header.offsetX += offsX;
	wb.header.offsetY += offsY;

	return VWB_ERROR_NONE;
}


VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = h.width * 4;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 32, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0 
	};
	BITMAPFILEHEADER bmfh = {
		'MB', 
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0, 
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) 
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; p++ )
	{
		VWB_byte c[4] = { p->b, p->g, p->r, p->a };
		os.write( (const char*)c, 4 );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_WarpRecord const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	// bmp stores pixels in packed rows, the size of each row is rounded up to a multiple of 4 bytes by padding
	// pitchBM = number of bytes necessary to store one row of pixels = rounded up to a multiple of 4
	const VWB_uint pitchBM = 4 * h.width;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),		// number of bytes required by the structure
		h.width,
		-h.height,						// biHeight is negative, the bitmap is a top-down DIB with the origin at the upper left corner.
		1,								// biPlanes, Specifies the number of planes for the target device. This value must be set to 1.
		32,								// biBitCount, Specifies the number of bits per pixel (bpp).
		0,								// biCompression
		0,								// biSizeImage, Specifies the size, in bytes, of the image. This can be set to 0 for uncompressed RGB bitmaps.
		5512,							// biXPelsPerMeter, horizontal resolution, in pixels per meter, 
		5512,							// biYPelsPerMeter, horizontal resolution, in pixels per meter, 
		0,								// biClrUsed, number of color indices in the color table 
		0								// biClrImportant, number of color indices that are considered important for displaying the bitmap. If this value is zero, all colors are important.
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	// iterate through the whole image line by line. 
	for( VWB_WarpRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE;)
	{
		// write one line, write until we are at the end
		for( VWB_WarpRecord const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_byte c[4] = { (VWB_byte)( 255.0f * p->z ), (VWB_byte)( 255.0f * p->y ), (VWB_byte)( 255.0f * p->x ), (VWB_byte)( 255.0f * p->w ) };
			os.write( (const char*)c, 4 );
		}
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_WarpRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.fail() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, std::ostream& os, VWB_word semantic )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = ( ( ( ( h.width * 24 ) + 31 ) & ~31 ) >> 3 );
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 24, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		semantic, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; )
	{
		for( VWB_BlendRecord const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_byte c[3] = { p->b, p->g, p->r };
			os.write( (const char*)c, 3 );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord2 const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = ( ( ( ( h.width * 48 ) + 31 ) & ~31 ) >> 3 );
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 48, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord2 const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; )
	{
		for( VWB_BlendRecord2 const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_word c[3] = { p->b, p->g, p->r };
			os.write( (const char*)c, 3 * sizeof( p->r ) );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord3 const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = ( ( ( ( h.width * 96 ) + 31 ) & ~31 ) >> 3 );
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 96, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord3 const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; )
	{
		for( VWB_BlendRecord3 const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_float c[3] = { p->b, p->g, p->r };
			os.write( (const char*)c, 3 * sizeof( p->r ) );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.fail() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP_RGBA( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.fail() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord2 const* map, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.fail() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord3 const* map, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.fail() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, std::ostream& os, const char* aesKey )
{
	if (os.bad())
		return VWB_ERROR_PARAMETER;

	VWB_WarpSetFileHeader hdrSet = { {'v','w','f','1'}, 0, sizeof( VWB_WarpSetFileHeader ), 0 };
	// count blocks
	for( auto setIt : set )
	{
		if( setIt->pWarp )
			hdrSet.numBlocks++;
		else
			return VWB_ERROR_GENERIC;
		if( setIt->pBlend )
			hdrSet.numBlocks++;
		if( setIt->pBlack )
			hdrSet.numBlocks++;
		if( setIt->pWhite )
			hdrSet.numBlocks++;
	}
	if( hdrSet.numBlocks )
	{
		os.write( (const char*)&hdrSet, sizeof( hdrSet ) );
		for( auto setIt : set )
		{
			if( setIt->pWarp )
			{
				if( aesKey && aesKey[0] )
					setIt->header.flags |= FLAG_WARPFILE_HEADER_ENCRYPTED;

				setIt->header.magicNumber[0] = 'v';
				setIt->header.magicNumber[1] = 'w';
				setIt->header.magicNumber[2] = 'f';
				setIt->header.magicNumber[3] = '0';
				setIt->header.szHdr = sizeof( setIt->header );
				os.write( (const char*)&setIt->header, setIt->header.szHdr );
				size_t sz = sizeof( VWB_WarpRecord ) * size_t( setIt->header.width ) * setIt->header.height;
				
				if( setIt->header.flags & FLAG_WARPFILE_HEADER_ENCRYPTED )
				{
					AES_ctx ctx;
					AES_init_ctx_iv( &ctx, (uint8_t*)aesKey, _iv );
					AES_CBC_encrypt_buffer( &ctx, (uint8_t*)setIt->pWarp, sz );
					setIt->header.szKey = 16;
					strcpy( setIt->header.keyIdent, "AES128" );
					AES_CBC_encrypt_buffer( &ctx, (uint8_t*)setIt->header.keyIdent, 16 );
					// memcpy( setIt->header.key, aesKey, sizeof( setIt->header.key ) ); // TODO just for testing !! remove writing plain key here!!
					strcpy( setIt->header.keyDesc, "encrypted with AES128" );
				}

				os.write( (const char*)setIt->pWarp, sz );
			}
			if( setIt->pBlend )
			{
				if( setIt->header.flags & FLAG_WARPFILE_HEADER_BLENDV3 )
					SaveBMP( setIt->header, setIt->pBlend3, os );
				else if( setIt->header.flags & FLAG_WARPFILE_HEADER_BLENDV2 )
					SaveBMP( setIt->header, setIt->pBlend2, os );
				else
					SaveBMP( setIt->header, setIt->pBlend, os );
			}
			if( setIt->pBlack )
			{
				SaveBMP( setIt->header, setIt->pBlack, os, 2 );
			}
			if( setIt->pWhite )
			{
				SaveBMP( setIt->header, setIt->pWhite, os, 3 );
			}
			if( os.bad() )
			{
				logStr( 0, "ERROR: SaveVWF: Error writing.\n" );
				return VWB_ERROR_VWF_FILE_NOT_FOUND;
			}

		}
		return VWB_ERROR_NONE;
	}
	else
	{
		logStr( 0, "ERROR: SaveVWF: no maps in set, nothing to save." );
		return VWB_ERROR_PARAMETER;
	}
}

VWB_ERROR SaveVWF(VWB_WarpBlendSet const& set, char const* path, const char* aesKey )
{
	char pp[MAX_PATH];
	strcpy_s(pp, path);
	MkPath(pp, MAX_PATH, ".vwf");
	std::ofstream os( pp, std::ios_base::binary );
	if (os.fail())
	{
		logStr(0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp);
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveVWF(set, os, aesKey );
}

VWB_rect& operator+=(VWB_rect& me, VWB_rect const& other )
{
	if( 0 == me.left && 0 == me.top && 0 == me.right && 0 == me.bottom )
		me = other;
	else
	{
		if( me.left > other.left )
			me.left = other.left;
		if( me.right < other.right )
			me.right = other.right;
		if( me.top > other.top )
			me.top = other.top;
		if( me.bottom < other.bottom )
			me.bottom = other.bottom;
	}
	return me;
}

VWB_rect operator+(VWB_rect const& me, VWB_rect const& other )
{
	VWB_rect r;
	if( 0 == me.left && 0 == me.top && 0 == me.right && 0 == me.bottom )
		r = other;
	else
	{
		if( me.left > other.left )
			r.left = other.left;
		else
			r.left = me.left;

		if( me.right < other.right )
			r.right = other.right;
		else
			r.right = me.right;

		if( me.top > other.top )
			r.top = other.top;
		else
			r.top = me.top;

		if( me.bottom < other.bottom )
			r.bottom = other.bottom;
		else
			r.bottom = me.bottom;
	}
	return r;
}

bool VerifySet( VWB_WarpBlendSet& set, int iScanIndex )
{
	for( VWB_WarpBlendSet::iterator it = set.begin(); it != set.end(); )
	{
		// check
		if( 0 != (*it)->header.hMonitor &&
			NULL != (*it)->pWarp &&
			0 != (*it)->header.width &&
			0 != (*it)->header.height )
		{
			if( (*it)->header.flags & FLAG_WARPFILE_HEADER_DISPLAY_SPLIT )
			{
				if( (*it)->header.flags & FLAG_WARPFILE_HEADER_OFFSET )
				{
					// we have a valid split, now look if we need to scale according to own compound
					// this happens, if we create several compounds on a mosaic, i.e. a power wall where there is just one big desktop monitor
					// find compound rect
					VWB_rect rCompound = {0};
					VWB_rect rScreen = rCompound;
					for( VWB_WarpBlendSet::iterator it2 = set.begin(); it2 != set.end(); it2++ )
					{
						if( ( (*it2)->header.flags & FLAG_WARPFILE_HEADER_OFFSET ) &&
							0 != (*it2)->header.hMonitor )
						{
							VWB_rect rDisplay = { (VWB_int)(*it2)->header.offsetX, (VWB_int)(*it2)->header.offsetY, (VWB_int)(*it2)->header.offsetX + (VWB_int)(*it2)->header.width, (VWB_int)(*it2)->header.offsetY + (VWB_int)(*it2)->header.height };
							if( (*it)->header.hMonitor == (*it2)->header.hMonitor ) 
								rScreen+= rDisplay;
							if( 0 == strcmp( (*it)->path, (*it2)->path ) )
								rCompound+= rDisplay;
						}
					}
					if( ( rCompound.right - rCompound.left ) < ( rScreen.right - rScreen.left ) ||
						( rCompound.bottom - rCompound.top ) < ( rScreen.bottom - rScreen.top ) )
					{
						// this is where we have to do something
						float scaleX = float(rCompound.right-rCompound.left) / float(rScreen.right-rScreen.left);
						float scaleY = float(rCompound.bottom-rCompound.top) / float(rScreen.bottom-rScreen.top);
						float offsX  = float(rCompound.left-rScreen.left) / float(rScreen.right-rScreen.left);
						float offsY  = float(rCompound.top-rScreen.top) / float(rScreen.bottom-rScreen.top);
						for( VWB_WarpRecord* pDst = (*it)->pWarp, *pDstE = pDst + (ptrdiff_t)(*it)->header.width * (ptrdiff_t)(*it)->header.height; pDst != pDstE; pDst++ )
						{
							if( 1 == pDst->z )
							{
								pDst->x*= scaleX;
								pDst->y*= scaleY;
								pDst->x+= offsX;
								pDst->y+= offsY;
							}
						}
					}
				}
			}
			else
			{
				// try to merge if display is already there
				for( VWB_WarpBlendSet::iterator it2 = it + 1; it2 != set.end(); )
				{
					if( (*it)->header.hMonitor == (*it2)->header.hMonitor &&
						0 != (*it2)->header.width &&
						0 != (*it2)->header.height &&
						(*it)->header.width == (*it2)->header.width &&
						(*it)->header.height == (*it2)->header.height &&
						(*it)->pWarp && (*it2)->pWarp )
					{
						for( ptrdiff_t d = 0, dE = (ptrdiff_t)(*it)->header.width * (ptrdiff_t)(*it)->header.height; d != dE; d++ )
						{
							VWB_WarpRecord* pW2 = ( *it2 )->pWarp + d;
							if( pW2->w > 0.0f ) // other set is filled here...
							{
								VWB_WarpRecord* pW1 = ( *it )->pWarp + d;
								// if this set is not filled here
								if( pW1->w == 0.0f )
								{
									*pW1 = *pW2;

									if( ( *it )->pBlend && ( *it2 )->pBlend )
									{
										if( FLAG_WARPFILE_HEADER_BLENDV3 & ( *it )->header.flags )
										{
											VWB_BlendRecord3* pB1 = ( *it )->pBlend3 + d;
											VWB_BlendRecord3* pB2 = ( *it2 )->pBlend3 + d;
											*pB1 = *pB2;
										}
										else if( FLAG_WARPFILE_HEADER_BLENDV2 & ( *it )->header.flags )
										{
											VWB_BlendRecord2* pB1 = ( *it )->pBlend2 + d;
											VWB_BlendRecord2* pB2 = ( *it2 )->pBlend2 + d;
											*pB1 = *pB2;
										}
										else
										{
											VWB_BlendRecord* pB1 = ( *it )->pBlend + d;
											VWB_BlendRecord* pB2 = ( *it2 )->pBlend + d;
											*pB1 = *pB2;
										}
									}

									if( ( *it )->pBlack && ( *it2 )->pBlack )
									{
										VWB_BlendRecord* pBl1 = ( *it )->pBlack + d;
										VWB_BlendRecord* pBl2 = ( *it2 )->pBlack + d;
										*pBl1 = *pBl2;
									}
									if( ( *it )->pWhite && ( *it2 )->pWhite )
									{
										VWB_BlendRecord* pWh1 = ( *it )->pWhite + d;
										VWB_BlendRecord* pWh2 = ( *it2 )->pWhite + d;
										*pWh1 = *pWh2;
									}
								}
								//TODO find uniformly filled parts
								// if this set is filled unifomly and the other is not
								//else if( 
							}
						}
						it2 = set.erase( it2 ); // does not invalitate it!
					}
					else
						it2++;
				}
			}
			it++;
		}
		else if( -1 == iScanIndex || it - set.begin() == iScanIndex )
		{
			it = set.erase( it );
			if( -1 != iScanIndex )
				break;
		}
		else
			it++;
	}
	return !set.empty();
}

VWB_ERROR PrepareForUse( VWB_WarpBlend& wb, const float gamma )
{
	// sanity check
	size_t sz = size_t( wb.header.width ) * wb.header.height;
	if( 1024 > sz )
	{
		logStr( 0, "ERROR: mapping too small. Probably a faulty mapping file.\n" );
		return VWB_ERROR_VWF_LOAD;
	}

	if( NULL == wb.pWarp )
	{
		logStr( 0, "ERROR: Warp texture missing. Seriously.\n" );
		return VWB_ERROR_WARP;
	}

	if( NULL == wb.pBlend )
	{
		logStr( 1, "WARNING: Blend texture missing. Creating...\n" );
		wb.pBlend2 = new VWB_BlendRecord2[sz];
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + sz; p != pE; p++ )
		{
			p->r = 65535;
			p->g = 65535;
			p->b = 65535;
		}
		wb.header.flags &= ~FLAG_WARPFILE_HEADER_BLENDV3;
		wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
	}

	// regardless of the given format, prepare blend as U16 NORM
	// if gamma is set, apply gamma and promote blend to VWB_BlendRecord2
	if( 0.0f < gamma && 1.0f != gamma )
	{
		logStr( 1, "Adapting gamma by %.5f\n", gamma );
		VWB_float g = 1.0f / gamma;
		if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 )
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[sz];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord3* p = wb.pBlend3, *pE = wb.pBlend3 + sz; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( pow( p->r, g ) * 65535.0f );
				pD->g = VWB_word( pow( p->g, g ) * 65535.0f );
				pD->b = VWB_word( pow( p->b, g ) * 65535.0f );
				pD->a = VWB_word( p->a * 65535.0f );
			}
			wb.header.flags &= ~FLAG_WARPFILE_HEADER_BLENDV3;
			wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend3;
			wb.pBlend2 = pDst;
		}
		else if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2 )
		{
			for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + sz; p != pE; p++ )
			{
				p->r = VWB_word( pow( VWB_float( p->r ) / 65535.0f, g ) * 65535.0f );
				p->g = VWB_word( pow( VWB_float( p->g ) / 65535.0f, g ) * 65535.0f );
				p->b = VWB_word( pow( VWB_float( p->b ) / 65535.0f, g ) * 65535.0f );
				// p->a stays untouched
			}
		}
		else
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[sz];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord* p = wb.pBlend, *pE = wb.pBlend + sz; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( pow( VWB_float( p->r ) / 255.0f, g ) * 65535.0f );
				pD->g = VWB_word( pow( VWB_float( p->g ) / 255.0f, g ) * 65535.0f );
				pD->b = VWB_word( pow( VWB_float( p->b ) / 255.0f, g ) * 65535.0f );
				pD->a = VWB_word( p->a ) * 255;
			}
			wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend;
			wb.pBlend2 = pDst;
		}
	}
	else if( !( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2 ) )
	{ // change to VWB_BlendRecord2 anyway...
		if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 )
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[sz];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord3* p = wb.pBlend3, *pE = wb.pBlend3 + sz; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( p->r * 65535.0f );
				pD->g = VWB_word( p->g * 65535.0f );
				pD->b = VWB_word( p->b * 65535.0f );
				pD->a = VWB_word( p->a * 65535.0f );
			}
			wb.header.flags &= ~FLAG_WARPFILE_HEADER_BLENDV3;
			wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend3;
			wb.pBlend2 = pDst;
		}
		else
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[sz];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord* p = wb.pBlend, *pE = wb.pBlend + sz; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( p->r ) * 257; // as 257*255 = 65535 = 100000001 * 11111111
				pD->g = VWB_word( p->g ) * 257;
				pD->b = VWB_word( p->b ) * 257;
				pD->a = VWB_word( p->a ) * 257;
			}
			wb.header.flags |= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend;
			wb.pBlend2 = pDst;
		}
	}

	// put clipping channel of warp to a channel of blend
	VWB_WarpRecord* pW = wb.pWarp;
	if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
	{
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + sz; p != pE; p++, pW++ )
		{
			p->a = VWB_word( pW->w * 65535.0f );
		}
	}
	else
	{
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + sz; p != pE; p++, pW++ )
		{
			p->a = VWB_word( pW->z * 65535.0f );
		}
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR ScanVWF(char const* path, VWB_WarpBlendHeaderSet* set)
{
	if(NULL == set)
	{
		logStr(0, "ERROR: VWB_vwfInfo: set is NULL.\n");
		return VWB_ERROR_PARAMETER;
	}
	DeleteVWF(*set);
	if( nullptr == path )
		set->shrink_to_fit(); // release the vector's memory

	VWB_WarpBlendSet wbs;
	if(path && path[0] )
		LoadVWF( wbs, path, true, -1 );

	for( auto const& d : wbs )
	{
		set->push_back( d );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR AddUnwarped2DTo( VWB_WarpBlendSet& set, const char* path, int xPos, int yPos, int width, int height, const char* displayName, int splitW, int splitH, int splitX, int splitY, VWB_uint hMonitor )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Missing path to warp file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= width || 0 >= height )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Need positive dimension.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( NULL == displayName || 0 == *displayName )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Missing display name.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= splitW || 0 >= splitH )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: split grid size out of range.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 == splitX || splitW <= splitX || 0 == splitY || splitH <= splitY )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: split grid index ot of range.\n" );
		return VWB_ERROR_PARAMETER;
	}

	auto wb = set.emplace_back( new VWB_WarpBlend() );

	VWB_ERROR ret = VWB_ERROR_NONE;
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".vwf" );

	// create basic set, no warping no blending
	VWB_uint nRecords = (VWB_uint)width * (VWB_uint)height;
	wb->header = VWB_WarpFileHeader5{
		{'v','w','f','0'},
		sizeof( VWB_WarpFileHeader5 ),
		FLAG_WARPFILE_HEADER_CALIBRATION_BASE_TYP|FLAG_WARPFILE_HEADER_OFFSET,
		0x10001,
		nRecords * (VWB_uint)sizeof( VWB_WarpRecord ),
		width,	height,
		{1,1,1,1},
		{0,0,0,0},
		(VWB_float)splitY,(VWB_float)splitX,(VWB_float)splitH,(VWB_float)splitW,VWB_float( splitW * width ),VWB_float( splitH * height ),
		TYP_CALIBBASE_DISPLAY_COMPOUND,
		(VWB_float)xPos,(VWB_float)yPos,
		0,0,0,
		0,
		{0,0,0},
		"dummy",
		{0},
		0,
		{0,0,(VWB_float)width,(VWB_float)height,1.0f/width,1.0f/height,VWB_float(width*height)},
		{0,0,1,1,1,0,0,1,1},
		{0},
		{0},
		{0},
		"localhost",
		0,
		{0},
		"",
		"not encrypted"
	};

	// wb->header = wfh;
	strcpy_s( wb->path, pp );

	wb->pBlend = new VWB_BlendRecord[nRecords];
	for( VWB_BlendRecord* pB = wb->pBlend, *pBE = pB + nRecords; pB != pBE; pB++ )
		pB->r = pB->g = pB->b = pB->a = 255;

	VWB_WarpRecord* pW = wb->pWarp = new VWB_WarpRecord[nRecords];
	int startX = splitX * width;
	int startY = splitY * height;
	int endX = startX + width;
	int endY = startY + height;

	int tW = splitW ? splitW * width : width;
	int tH = splitH ? splitH * height : height;

	VWB_float dX = 0.5f / tW; // midpix
	VWB_float dY = 0.5f / tH;
	VWB_float sX = 1.0f / VWB_float( tW );
	VWB_float sY = 1.0f / VWB_float( tH );
	for( int y = startY; y != endY; y++ )
	{
		VWB_float fy = VWB_float( y ) * sY + dY;
		for( int x = startX; x != endX; x++, pW++ )
		{
			pW->x = VWB_float( x ) * sX + dX;
			pW->y = fy;
			pW->z = 1;
			pW->w = 0;
		}
	}
	wb->pBlack = nullptr;
	wb->pWhite = nullptr;
	logStr( 2, "INFO: AddUnwarped2DToVWF: Set \"%s\" added.\n", displayName );

	return ret;
}

VWB_ERROR AddBlacklevelTo( VWB_WarpBlend& wb, VWB_BlendRecord const* blacklevelMap, float scale, float dark, float bright )
{
	if( NULL == blacklevelMap )
	{
		logStr( 0, "ERROR: AddBlacklevelTo: blacklevelMap must not be null.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= wb.header.width || 0 >= wb.header.height )
	{
		logStr( 0, "ERROR: AddBlacklevelTo: Width and hight in wb.header must be set.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( wb.pBlack )
		delete[] wb.pBlack;
	wb.pBlack = new VWB_BlendRecord[wb.header.width * wb.header.height];
	memcpy( wb.pBlack, blacklevelMap, wb.header.width * wb.header.height * sizeof( VWB_BlendRecord ) );
	wb.header.flags |= FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR;
	wb.header.blackScale = scale;
	wb.header.blackDark = dark;
	wb.header.blackBright = bright;
	return VWB_ERROR_NONE;
}

VWB_ERROR AddBlacklevelTo( VWB_WarpBlend& wb, float const* blacklevelMapRaw, float dark, float bright )
{
	if( NULL == blacklevelMapRaw )
	{
		logStr( 0, "ERROR: AddBlacklevelTo: blacklevelMap must not be null.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= wb.header.width || 0 >= wb.header.height )
	{
		logStr( 0, "ERROR: AddBlacklevelTo: Width and hight in wb.header must be set.\n" );
		return VWB_ERROR_PARAMETER;
	}
	float scale = 0;
	for( auto const* f = blacklevelMapRaw, *fE = f + 3 * wb.header.width * wb.header.height; f != fE; f++ )
	{
		if( scale < *f )
			scale = *f;
	}
	if( wb.pBlack )
		delete[] wb.pBlack;
	wb.pBlack = new VWB_BlendRecord[wb.header.width * wb.header.height];
	auto b = wb.pBlack;
	for( auto const* f = blacklevelMapRaw, *fE = f + 3 * wb.header.width * wb.header.height; f != fE; f+= 3, b++ )
	{
		b->r = ( uint8_t )( f[0] / scale * 255 );
		b->g = ( uint8_t )( f[1] / scale * 255 );
		b->b = ( uint8_t )( f[2] / scale * 255 );
		b->a = 0;
	}

	wb.header.flags |= FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR;
	wb.header.blackScale = scale;
	wb.header.blackDark = dark;
	wb.header.blackBright = bright;

	return VWB_ERROR();
}


