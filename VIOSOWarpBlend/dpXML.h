// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause


#include "../Include/VWBTypes.h"
#include <iostream>
#include <filesystem>

/// @brief LoadDP loads domeprojection files to the mapping
/// @param [IN|OUT] wb the warp blend to load the domeprojection into
/// @param [IN] paths a vector of filesystam::path to the domeprojection files
/// @return VWB_ERROR_NONE in case of success, an error code otherwise @see VWB_ERROR
VWB_ERROR LoadDPXML( VWB_WarpBlendSet& set, std::vector<std::filesystem::path> const& paths, bool flipVertices, bool flipUvs );