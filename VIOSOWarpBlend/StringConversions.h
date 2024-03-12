/// @file String conversions.
/// note: All conversions work with or without terminating '\0'. Still it is considered wrong, if the terminator is part of the string.
/// NOTE: when calling a wstring conversion first time, the locale is changed to system default with std::setlocale( LC_ALL, "" ).
/// You consider calling _initLocale() at the begin of the process if you want other locale set. 
/// You can switch off auto initialization by #define VWB_NO_AUTOLOCALE
/// At program start you are at minimal locale "C", which does not have characters above 127. All conversions of special characters will fail!

#pragma once
#include <string>
#include <cuchar>
#include <cstdlib>
#include <locale>
namespace VWBUtil
{
    /// returns a static system locale
    inline std::locale& my_locale() {
        static std::locale loc( "" );
        return loc;
    }
    /// returns a static ctype facet for single-byte character to wchar_t conversion
    inline auto& my_wchar_facet() {
        static auto const* fac = &std::use_facet<std::ctype<std::wstring::value_type> >( my_locale() );
        return fac;
    }

    /// set a different locale for my conversions
    /// Note: It does not work with multibyte locales the way you might expect
    /// all conversions which do not have a single byte equivalent will fail.
    inline void set_my_locale( std::locale&& loc )
    {
        my_locale() = std::move( loc );
        my_wchar_facet() = &std::use_facet<std::ctype<std::wstring::value_type> >( my_locale() );
    }

    /// helper to use best utf-8 to wchar
    template< size_t I >
    inline size_t __mbrtoc( void*, char*, size_t, std::mbstate_t* ) { return -1; }
    template<>
    inline size_t __mbrtoc<2>( void* uc, char* u8c, size_t n, std::mbstate_t* state ) { return std::mbrtoc16( ( char16_t* )uc, u8c, n, state ); }
    template<>
    inline size_t __mbrtoc<4>( void* uc, char* u8c, size_t n, std::mbstate_t* state ) { return std::mbrtoc32( ( char32_t* )uc, u8c, n, state ); }
    /// helper to use best utf-8 to wchar
    template< class T >
    inline size_t _mbrtoc( T* uc, char* u8c, size_t n, std::mbstate_t* state ) { return __mbrtoc<sizeof(T)>( uc, u8c, n, state ); }

    ///  converts a locale single-byte character string to wstring
    inline std::wstring to_wstring( std::string const& s )
    {
        auto sz = s.size();
        std::wstring ws; ws.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto c : s )
        {
            if( !( 0x80 & c ) ) 
            {
                ws.push_back( std::wstring::value_type( c++ ) );
            }
            else
            {
                std::wstring::value_type cv = my_wchar_facet()->widen( c );
                if( cv )
                {
                    ws.push_back( cv );
                }
            }
        }
        return ws;
    }

    ///  converts an u8string to wstring
    inline std::wstring to_wstring( std::u8string const& u8s )
    {
        auto sz = u8s.size();
        std::wstring ws; ws.reserve( sz );
        for( auto c = (char*)u8s.data(), cE = c + sz; c < cE; )
        {
            // try to be quick, if all chars are simple ASCII
            if( !( 0x80 & *c ) )
            {
                ws.push_back( std::wstring::value_type( *( c++ ) ) );
            }
            else
            {
                wchar_t wc = 0;
                std::mbstate_t state{};
                size_t a = _mbrtoc( &wc, c, cE - c, &state );
                if( 4 < a )
                { // error, surrogate or inclomplete
                    c++;
                }
                else
                {
                    ws.push_back( std::wstring::value_type( wc ) );
                    c += a;
                }
            }
        }
        return ws;
    }

    /// converts a wstring to string on current locale single-byte characters
    inline std::string to_string( std::wstring const& ws )
    {
        auto sz = ws.size();
        std::string s; s.reserve( sz );
        for( auto wc : ws )
        {
            // try to be quick, if all chars are simple ASCII
            if( 0x80 > wc )
            {
                s.push_back( std::string::value_type( wc ) );
            }
            else
            {
                auto c = my_wchar_facet()->narrow( wc );
                if( c )
                    s.push_back( c );
            }
        }
        return s;
    }

    /// converts a wstring to u8string
    inline std::u8string to_u8string( std::wstring const& ws )
    {
        auto sz = ws.size();
        std::u8string u; u.reserve( sz );
        for( auto wc : ws )
        {
            // try to be quick, if all chars are simple ASCII
            if( 0x80 > wc )
            {
                u.push_back( std::string::value_type( wc ) );
            }
            else
            {
                char u8c[4];
                std::mbstate_t state{};
                size_t a = std::c16rtomb( u8c, wc, &state );
                if( 4 >= a )
                    u.append( u8c, u8c + a );
            }
        }
        return u;
    }

    /// converts a utf-8 string to locale codepage single-byte character set
    /// Note: There is a problem with codepoints above 0xFEFF. 
    inline std::string to_string( std::u8string const& u8s )
    {
        auto sz = u8s.size();
        std::string s; s.reserve( sz );
        for( auto u8c = ( char* )u8s.data(), cE = u8c + sz; u8c < cE; )
        {
            // try to be quick, if all chars are simple ASCII
            if( !( 0x80 & *u8c ) )
            {
                s.push_back( char( *(u8c++) ) );
            }
            else
            {
                wchar_t uc;
                std::mbstate_t state{};
                // this encodes to UTF-32 which is nearly same as UCS4 char but without multi char surrogates.
                size_t a = _mbrtoc( &uc, u8c, u8c - cE, &state );
                if( 4 >= a )
                {
                    u8c += a;
                    auto c = my_wchar_facet()->narrow( wchar_t( uc ), 0 );
                    if( c )
                        s.push_back( c );
                }
                else
                    u8c++;
            }
        }

        return s;
    }

    ///  converts a locale single-byte character string to utf-8 string
    inline std::u8string to_u8string( std::string const& s )
    {
        auto sz = s.size();
        std::u8string u8s; u8s.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto c : s )
        {
            if( !( 0x80 & c ) )
            {
                u8s.push_back( char8_t( c ) );
            }
            else
            {
                auto wc = my_wchar_facet()->widen( c );
                if( wc )
                {
                    char u8c[4];
                    std::mbstate_t state{};
                    size_t b = std::c32rtomb( u8c, wc, &state );
                    if( 4 >= b )
                        u8s.append( u8c, u8c + b );
                }
            }
        }

        return u8s;
    }

    #ifdef _TCHAR_DEFINED
        #ifdef  UNICODE 
            using tstring = std::wstring;
            #define to_tstring to_wstring
        #else // def UNICODE
            using tstring = std::string;
            #define to_tstring to_string
        #endif // def UNICODE
        // to make tstring wor we need identity conversions
        constexpr std::string to_string( std::string const& s ) { return s; };
        constexpr std::wstring to_wstring( std::wstring const& s ) { return s; };
        #define TSTRING( a ) tstring( _T( a ) )
    #endif // def _TCHAR_DEFINED
}; // namespace VWBUtil
