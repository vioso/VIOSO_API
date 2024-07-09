// VIOSO API libpng wrapper
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "png.h"
#include <exception>
#include <vector>
#include <fstream>

class CPng
{
public:
    png_uint_32 m_width;
    png_uint_32 m_height;
    int m_bitDepth;
    int m_colorType;
    std::vector<uint8_t> m_imageData;
private:
    png_structp m_pPng;
    png_infop m_pInfo;
    int m_interlace;
    int m_compression;
    int m_filter;

    struct Io {
        uint8_t* data;
        size_t size;
        ptrdiff_t off;
        Io( uint8_t* _data, size_t _size ) : data( _data ), size( _size ), off( 0 ) {}
    };

    struct Fio {
        std::ifstream& f;
        Fio( std::ifstream& _f ) : f( _f ) {}
    };

    static void rdfn( png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead )
    {
        Io* io = (Io*)png_get_io_ptr( png_ptr );
        if( !io )
            throw std::exception( "png read parameter 0" );
        if( io->size < io->off + byteCountToRead )
            throw std::exception( "png read out of bound" );
        memcpy( outBytes, io->data + io->off, byteCountToRead );
        io->off += byteCountToRead;
    }

    static void frdfn( png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead )
    {
        Fio* io = (Fio*)png_get_io_ptr( png_ptr );
        if( !io || io->f.bad() )
            throw std::exception( "png read parameter bad" );
        if( io->f.read( (char*)outBytes, byteCountToRead ).fail() )
            throw std::exception( "png read out of bound" );
    }

public:
    // read from memory
	CPng( uint8_t* data, size_t size )
        : m_width{}
        , m_height{}
        , m_bitDepth{}
        , m_colorType{}
        , m_pPng{}
        , m_pInfo{}
        , m_interlace{}
        , m_compression{}
        , m_filter{}
 
	{
        if( !png_check_sig( (png_byte const*)data, 8 ) )
            throw std::exception( "unknown png signature" );
        m_pPng = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL,
                                            NULL );
        if( !m_pPng )
            throw std::exception( "failed to create png read struct" );
        m_pInfo = png_create_info_struct( m_pPng );
        if( !m_pInfo )
            throw std::exception( "failed to create png info struct" );
        Io io( data + 8, size );
        png_set_read_fn( m_pPng, &io, &rdfn );

        png_set_sig_bytes( m_pPng, 8 );
        png_read_info( m_pPng, m_pInfo );
        png_get_IHDR( 
            m_pPng, m_pInfo,
            &m_width,
            &m_height,
            &m_bitDepth,
            &m_colorType,
            &m_interlace,
            &m_compression,
            &m_filter 
        );
        if( PNG_INTERLACE_NONE != m_interlace )
            throw std::exception( "interlaced image read not implemented" );
        if( PNG_FILTER_TYPE_BASE != m_interlace )
            throw std::exception( "filtered image read not implemented" );
        if( PNG_COLOR_MASK_PALETTE & m_colorType )
            throw std::exception( "palette image read not implemented" );
        if( 8 > m_bitDepth || 16 < m_bitDepth )
            throw std::exception( "only 8bit or 16bit color depthpermitted" );

        size_t ch;
        if( m_colorType & PNG_COLOR_MASK_COLOR )
            ch = 3;
        else
            ch = 1;
        if( m_colorType & PNG_COLOR_MASK_ALPHA )
            ch++;

        size_t lDst = size_t( m_width ) * ch * m_bitDepth / 8;
        size_t s = lDst * m_height;
        m_imageData.resize( s );

        for( auto px = m_imageData.data(), pxE = px + s; px != pxE; px += lDst )
        {
            png_read_row( m_pPng, px, nullptr );
        }
	}

    // read from file
    CPng( char const* path )
        : m_width{}
        , m_height{}
        , m_bitDepth{}
        , m_colorType{}
        , m_pPng{}
        , m_pInfo{}
        , m_interlace{}
        , m_compression{}
        , m_filter{}
    {
        std::ifstream f( path, std::ios::in | std::ios::binary );
        if( !f.is_open() )
            throw std::exception( "file not found" );
        Fio io( f );
        

        uint8_t sig[8];
        f.read( (char*)sig, 8 );

        if( !png_check_sig( (png_byte const*)sig, 8 ) )
            throw std::exception( "unknown png signature" );
        m_pPng = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL,
                                         NULL );
        if( !m_pPng )
            throw std::exception( "failed to create png read struct" );
        m_pInfo = png_create_info_struct( m_pPng );
        if( !m_pInfo )
            throw std::exception( "failed to create png info struct" );
        png_set_read_fn( m_pPng, &io, &frdfn );

        png_set_sig_bytes( m_pPng, 8 );
        png_read_info( m_pPng, m_pInfo );
        png_get_IHDR(
            m_pPng, m_pInfo,
            &m_width,
            &m_height,
            &m_bitDepth,
            &m_colorType,
            &m_interlace,
            &m_compression,
            &m_filter
        );
        if( PNG_INTERLACE_NONE != m_interlace )
            throw std::exception( "interlaced image read not implemented" );
        if( PNG_FILTER_TYPE_BASE != m_interlace )
            throw std::exception( "filtered image read not implemented" );
        if( PNG_COLOR_MASK_PALETTE & m_colorType )
            throw std::exception( "palette image read not implemented" );
        if( 8 > m_bitDepth || 16 < m_bitDepth )
            throw std::exception( "only 8bit or 16bit color depthpermitted" );

        size_t ch;
        if( m_colorType & PNG_COLOR_MASK_COLOR )
            ch = 3;
        else
            ch = 1;
        if( m_colorType & PNG_COLOR_MASK_ALPHA )
            ch++;

        size_t lDst = size_t( m_width ) * ch * m_bitDepth / 8;
        size_t s = lDst * m_height;
        m_imageData.resize( s );

        for( auto px = m_imageData.data(), pxE = px + s; px != pxE; px += lDst )
        {
            png_read_row( m_pPng, px, nullptr );
        }

    }

	~CPng()
	{
        png_destroy_info_struct( m_pPng, &m_pInfo );
        png_destroy_read_struct( &m_pPng, nullptr, nullptr );
    }

    void convertRGB8toRGBA8( uint8_t* out ) const
    {
        uint8_t const* in = m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = *in++;
            *o++ = *in++;
            *o++ = *in++;
            *o++ = 255;
        }
    }

    void convertRGB16toRGBA8( uint8_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = 255;
        }
    }

    void convertRGBA16toRGBA8( uint8_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
        }
    }

    void convertRGB16toRGBA16( uint16_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * 3; in != e; )
        {
            *o++ = *in++;
            *o++ = *in++;
            *o++ = *in++;
            *o++ = 65535;
        }
    }

};

#pragma comment( lib, "lpng16/libpng16_static.lib")
#pragma comment( lib, "zlib/zlibstatic.lib")
