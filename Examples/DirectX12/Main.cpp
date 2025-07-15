//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12HelloTexture.h"
#include "../../VIOSOWarpBlend/logging.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	strcpy_s( g_logFilePath, "D3D12Example.log" );
    try {
        D3D12HelloTexture sample( 1280, 720, L"Display1" );
        return Win32Application::Run( &sample, hInstance, nCmdShow );
	} catch( const HrException& e ) {
		logStr( 0, "Error: %s (HRESULT: 0x%08X)\n", e.what(), e.Error() );
		return -1;
	} catch( const std::exception& e ) {
		logStr( 0, "Error: %s\n", e.what() );
		return -1;
	} catch( ... ) {
		logStr( 0, "Unknown error occurred.\n" );
		return -1;
	}
}
