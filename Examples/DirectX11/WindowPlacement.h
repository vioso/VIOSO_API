// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once
#include "DX11Program.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
// VWBTypes.h must come after the standard headers
#include "../../Include/VWBTypes.h"

// Where the windows of a vwf's channels go: as stored in the vwf, if the whole arrangement is on the desktop.
// Otherwise, or with --small (1/4), the arrangement is shrunk evenly, so the relative positions stay, and
// centered in the work area of the primary monitor.
struct Placement
{
    double scale = 1;
    double ox = 0, oy = 0; // where the origin of the vwf's coordinates goes
    std::wstring note;          // appended to the window titles, if the layout differs from the vwf's

    Placement( std::vector<VWB_WarpBlendHeader> const& set, bool preview )
    {
        if( preview )
        {
            scale = 0.25;
            note = L" (preview, 1/4)";
            return;
        }
        if( set.empty() )
            return;
        double l = set[0].header.offsetX, t = set[0].header.offsetY, r = l, b = t;
        for( auto const& h : set )
        {
            l = ( std::min )( l, double( h.header.offsetX ) );
            t = ( std::min )( t, double( h.header.offsetY ) );
            r = ( std::max )( r, double( h.header.offsetX ) + h.header.width );
            b = ( std::max )( b, double( h.header.offsetY ) + h.header.height );
        }
        int vl = GetSystemMetrics( SM_XVIRTUALSCREEN ), vt = GetSystemMetrics( SM_YVIRTUALSCREEN );
        int vr = vl + GetSystemMetrics( SM_CXVIRTUALSCREEN ), vb = vt + GetSystemMetrics( SM_CYVIRTUALSCREEN );
        if( l >= vl && t >= vt && r <= vr && b <= vb )
            return; // fits the desktop as is
        RECT wa{ 0, 0, 1024, 768 };
        SystemParametersInfo( SPI_GETWORKAREA, 0, &wa, 0 ); // work area of the primary monitor
        double ww = wa.right - wa.left, wh = wa.bottom - wa.top, bw = r - l, bh = b - t;
        if( bw <= 0 || bh <= 0 )
            return;
        scale = ( std::min )( { 1.0, ww / bw, wh / bh } );
        ox = wa.left + ( ww - bw * scale ) / 2 - l * scale;
        oy = wa.top + ( wh - bh * scale ) / 2 - t * scale;
        note = scale < 0.995 ? L" (preview, scaled to fit, " + std::to_wstring( int( scale * 100 + 0.5 ) ) + L"%)" : L" (preview, moved to fit)";
    }

    RECT rect( VWB_WarpBlendHeader const& h ) const
    {
        LONG x = lround( ox + h.header.offsetX * scale ), y = lround( oy + h.header.offsetY * scale );
        LONG w = ( std::max )( 1L, lround( h.header.width * scale ) ), hi = ( std::max )( 1L, lround( h.header.height * scale ) );
        return { x, y, x + w, y + hi };
    }
};
