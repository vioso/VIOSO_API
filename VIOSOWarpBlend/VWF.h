// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

/// Used to read and write .vwf files
/// A vwf file consists of a set header and chunks
/// Basic format of info:
/// FileSetHeader
/// All values must be set:
/// VWB_WarpSetFileHeader.magicNumber = '1fwv';
/// VWB_WarpSetFileHeader.numBlocks = numberOfAllWarpDataSets + numberOfAllBlendMaps + numberOfAllBlacklevelMaps;
/// VWB_WarpSetFileHeader.off = dataOffset; // = sizeof( VWB_WarpSetFileHeader ) = 16;
/// VWB_WarpSetFileHeader.reserved = 0;
/// at absolute file position VWB_WarpSetFileHeader.off, write the first chunk which must be warp information, starting with a VWB_WarpFileHeader
/// Mandatory header values:
/// VWB_WarpFileHeader5.magicNumber = '0fwv';
/// VWB_WarpFileHeader5.szHdr = sizeof( VWB_WarpFileHeader5 );
/// VWB_WarpFileHeader5.width = w;
/// VWB_WarpFileHeader5.height = h;
/// Optional values
/// VWB_WarpFileHeader5.name = "channelName";
/// VWB_WarpFileHeader5.primName = "screenName";
/// VWB_WarpFileHeader5.hostName = "clientName";
/// VWB_WarpFileHeader5.vPartialCnt[0] = normalizedLeft; // 0.4   for content rect
/// VWB_WarpFileHeader5.vPartialCnt[0] = normalizedTop; // 0.1
/// VWB_WarpFileHeader5.vPartialCnt[0] = normalizedRight; // 0.7
/// VWB_WarpFileHeader5.vPartialCnt[0] = normalizedBottom; // 0.3
/// next is binary warp data: VWB_WarpRecord[width x height] 
///  midle-of-pixel aligned normalized lookup. 
///  2D: VWB_WarpRecord.{x = u; y = v; z = valid ? 1 : 0; w = 0}
///  3D: VWB_WarpFileHeader.flags|= FLAG_WARPFILE_HEADER_3D; VWB_WarpRecord.{x = X; y = Y; z = Z; w = valid ? 1 : 0}
/// next chunk if not "vwf" is blend:
/// windows bitmap, set VWB_WarpFileHeader.flags|= FLAG_WARPFILE_HEADER_BLENDV2, if 16bit per channel; set VWB_WarpFileHeader.flags|= FLAG_WARPFILE_HEADER_BLENDV3, if 32 bit per channel
/// the bitmap might be set to size 0 x 0, to indicate no blend map
/// next chunk if not "vwf" is blacklevel:
/// windows bitmap, always RGB8U, optional, set VWB_WarpFileHeader.flags|= FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR; if present
///  set VWB_WarpFileHeader4.{blackScale,blackDark,blackBright}.
///  shader code:
///  vec4 black = _tex2D( samBlack, texcoord.st ) * blackScale;
///  FragColor *= vec4(1,1,1,1) - backDark * blackBright * black;
///  FragColor += blackDark * black;          
///  FragColor = max( FragColor, black );        
///    blackDark == 0 => every pixel is set to blacklevel if below
///    blackDark == 1 => pixel is uplifted above blacklevel, which might be above 1
///    blackBright == 0 => pixel is clamped to 1
///    blackBright == 1 => pixel is scaled to [blacklevel..1]
///   normally blackDark = blackBright = 1, but you'll loose color resolution. It might be better in dark scenario to just shift up, or in bright scenario to clamp lower.
/// next chunk if not "vwf" is white (ambient correction), not used so far
/// next chunk must be "vwf" for next channel
/// (c) VIOSO GmbH 2011 - 2024

#include "../Include/VWBTypes.h"
#include <iostream>

/// @brief free memory used by a VWB_WarpBlend structure
/// @param wb [INOUT] the warp blend to free
/// @param set [INOUT] the warp blend set to free
/// @param set [INOUT] the warp blend header set to free
/// @return true on success, false if wb is NULL or malformed
bool DeleteVWF( VWB_WarpBlend& wb );
bool DeleteVWF( VWB_WarpBlendSet& set );
bool DeleteVWF( VWB_WarpBlendHeaderSet& set );

/// @brief load or scan vwf file
/// @param set [OUT] the scanned set
/// @param path [IN] a comma separated list of paths to load
/// @param bScanOnly [OPT|IN] scan only, if set to true, the actual loading is skipped; all data pointers are NULL. This is used to analyze the mappings and assign them to a specific channel by names and ident information in header
/// @param [OPT|IN] a single index to load data. Do this after scanning, to save memory and loading time. The set headers are all filled, but mappings only loaded for that particular index. Set to -1 to load all.
/// @param aesKey [OPT|IN] if set, encryption indication flag is set; must be char[16] array
/// @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER, if path null or empty string, VWB_ERROR_VWF_FILE_NOT_FOUND, if path can't be found or opened, VWB_ERROR_VWF_LOAD, if malformed, VWB_ERROR_GENERIC otherwise
VWB_ERROR LoadVWF( VWB_WarpBlendSet& set, char const* path, bool bScanOnly = false, int loadIndex = -1, const uint8_t* aesKey = nullptr );

/// @brief split a warpblend, 
/// @param wb [INOUT] the warpbend to be split
/// @return VWB_ERROR_NONE on success, VWB_ERROR_FALSE, if calibSplit is indicating no split, 
/// VWB_ERROR_PARAMETER if set contains no data or the split will lead to uneven map (the width and hight must be devidable by the split numbers), or the split index is out of range (must be smaller than split number)
/// VWB_ERROR_GENERIC otherwise
VWB_ERROR SplitVWF(VWB_WarpBlend& wb, const VWB_word (& calibSplit)[4]);

VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, std::ostream& os, VWB_word semantic = 0 );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord2 const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord2 const* map, std::ostream& os );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord3 const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_BlendRecord3 const* map, std::ostream& os );
VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, std::ostream& os, const char* aesKey = nullptr );
VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, char const* path, const char* aesKey = nullptr );

///< @remark use SaveBMP   VWB_WarpRecord only for debug reasons, it will save 8 bit RGB only!
VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, std::ostream& os );
VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader5 const& h, VWB_BlendRecord const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_WarpRecord const* map, std::ostream& os );
VWB_ERROR SaveBMP( VWB_WarpFileHeader5 const& h, VWB_WarpRecord const* map, char const* path );

/// @brief merges warp and blend information from the set into a single VWB_WarpBlend structure, if the same monitor is addressed
/// it also offsets and scales the warp if a split is partially used
/// @param the warp blend set to verify
/// @param iScanIndex a specific index to verify, if set to -1, all indices are verified
/// @return 
bool VerifySet( VWB_WarpBlendSet& set, int iScanIndex = -1 );

/// @brief scan a vwf file, to get the warp and blend information, but not loading the data
/// @param path [IN] the path to the vwf file to scan
/// @param set [OUT] the set to fill with the warp and blend information
VWB_ERROR ScanVWF( char const* path, VWB_WarpBlendHeaderSet* set );

/// @brief upgrade set to be loaded to shader
/// * blend gets changed into U16 format
/// * validity channel (warp.z for 2D or w for 3D mappings) is copied to blend.a.
/// * gamma is applied to blend
/// * an empty blend map is created, in case there is no blend map
/// @param [IN|OUT] set				the set to upgrade
/// @param [IN|OPT] gamma  the used gamma value
/// @return VWB_ERROR_NONE in case of success 
VWB_ERROR PrepareForUse( VWB_WarpBlend& wb, const float gamma = 0 );

/// @brief	Calculate the bounds of the warp map.
/// @param [IN] wb the warp blend to calculate the bounds for.
/// @param [OUT] resX the optimal resolution in X direction.
/// @param [OUT] resY the optimal resolution in Y direction.
/// @param [OUT] minX the minimum X coordinate of the warp map.
/// @param [OUT] minY the minimum Y coordinate of the warp map.
/// @param [OUT] maxX the maximum X coordinate of the warp map.
/// @param [OUT] maxY the maximum Y coordinate of the warp map.
/// @return VWB_ERROR_NONE on success, VWB_ERROR_WARP if the warp map of the set is missing or malformed
VWB_ERROR CalculateBounds( VWB_WarpBlend& wb, int& resX, int& resY, int& minX, int& minY, int& maxX, int& maxY );

/// @brief creates an unwarped mapping and adds it to the given set. 
/// All warp maps are "bypass", that means they mimic an unwarped display, all pixels used
/// The split values are set in the split info and used to fill the warp lookup scaled and offsetted.
/// @param [OUT] set				the set to add
/// @param [IN] path				the path, the mapping was loaded from, must not be empty. Will be expanded to a valid path. Can be used later to save.
/// @param [IN] xPos, yPos			the desktop position (upper left corner) of the display part
/// @param [IN] width, height       size of the display in desktop coordinates
/// @param [IN] displayName			the name of the display
/// @param [IN] splitW				the number of columns in the split
/// @param [IN] splitH				the number of rows in the split
/// @param [IN] splitX				the split display's column index
/// @param [IN] splitY				the split display's row index
/// @param [IN] hMonitor			the monitor handle, if 0, the first monitor is used
/// @return VWB_ERROR_NONE in cas of success, VWB_ERROR_PARAMETER if some parameter is out of bound
VWB_ERROR AddUnwarped2DTo( VWB_WarpBlendSet& set, const char* path, int xPos, int yPos, int width, int height, const char* displayName, int splitW = 1, int splitH = 1, int splitX = 0, int splitY = 0, VWB_uint hMonitor = 0 );

/// @brief Adds or replaces a blacklevel (beta) map.
/// It sets FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR to wb.header.flags
/// (1) Sets wb.header.blackScale to scale, wb.header.blackDark to dark and wb.header.blackBright to bright.
/// (2) Creates a normalized blacklevel map (it is scaled the way the former maximum becomes 255) and wb.header.blackScale is set to scale it back properly. wb.header.blackDark is set to dark and wb.header.blackBright to bright.
/// @param [IN|OUT] wb				the VWB_WarpBlend to add the blacklevel map to
/// @param blacklevelMap			a blacklevel map. Must contain width * height entries.
/// @param blacklevelMapRaw			a blacklevel map in float format, interleaved RGB. Must contain 3 * width * height entries. 
/// @return VWB_ERROR_NONE in cas of success, VWB_ERROR_PARAMETER if some parameter is out of bound
VWB_ERROR AddBlacklevelTo( VWB_WarpBlend& wb, VWB_BlendRecord const* blacklevelMap, float scale = 1.0f, float dark = 1.0f, float bright = 1.0f );
VWB_ERROR AddBlacklevelTo( VWB_WarpBlend& wb, float const* blacklevelMapRaw, float dark = 1.0f, float bright = 1.0f );

