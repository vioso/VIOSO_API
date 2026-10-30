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

// Errors go to stderr and to a message box, the window is not there yet or already gone.
static int ReportFailure( char const* text )
{
	fprintf( stderr, "%s\n", text );
	MessageBoxA( NULL, text, "VIOSO DirectX 12 example", MB_OK | MB_ICONERROR );
	return 2;
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try {
        D3D12HelloTexture sample( 1280, 720, L"Display1" );
        return Win32Application::Run( &sample, hInstance, nCmdShow );
	} catch( const HrException& e ) {
		char msg[512];
		sprintf_s( msg, "Direct3D 12 failed: %s (HRESULT: 0x%08X)", e.what(), (unsigned)e.Error() );
		return ReportFailure( msg );
	} catch( const std::exception& e ) {
		return ReportFailure( e.what() );
	} catch( ... ) {
		return ReportFailure( "Unknown error occurred." );
	}
}
