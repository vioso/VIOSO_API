/// @file String conversions.
/// note: All conversions work with or without terminating '\0'. Still it is considered wrong, if the terminator is part of the string.
/// NOTE: when calling a wstring conversion first time, the locale is changed to system default with std::setlocale( LC_ALL, "" ).
/// You consider calling _initLocale() at the begin of the process if you want other locale set. 
/// You can switch off auto initialization by #define VWB_NO_AUTOLOCALE
/// At program start you are at minimal locale "C", which does not have characters above 127. All conversions of special characters will fail!

#pragma once
#include <string>
#include <string_view>
#include <cuchar>
#include <cstdlib>
#include <cstdio>
#include <locale>
#include <stdarg.h>
#include <algorithm>
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
                auto c = my_wchar_facet()->narrow( wc, '\0' );
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

    inline std::string fmt( const char* format, ... )
    {
        char buf[256];
        va_list args;
        va_start( args, format );
        const auto r = std::vsnprintf( buf, sizeof buf, format, args );
        if( r < 0 )
        {
            va_end( args );
            return {};
        }
        else
        {
            const size_t len = r;
            if( len < sizeof( buf ) )
            {
                va_end( args );
                return { buf, len };
            }

            std::string s( len, '\0' );
        #if __cplusplus >= 201703L
            std::vsnprintf( s.data(), len + 1, format, args );
        #else
            std::vsnprintf( &s[0], len + 1, format, args );
        #endif
            va_end( args );
            return s;
        }
    }

	template<typename CharT, typename Src>
		requires std::constructible_from<std::basic_string_view<CharT>, Src>
	inline CharT* copy( CharT* dst, std::size_t dstSz, Src&& src ) noexcept {
		if( dstSz == 0 ) return dst;

		// construct a string_view from src (works for const CharT*, std::string, std::string_view, ...)
		std::basic_string_view<CharT> sv{ std::forward<Src>( src ) };

		const std::size_t n = std::min<std::size_t>( dstSz - 1, sv.size() );
		std::copy_n( sv.begin(), n, dst ); // C++20 ranges
		dst[n] = CharT{}; // null-terminate
		return dst;
	}

	template<typename CharT, std::size_t N, typename Src>
		requires std::constructible_from<std::basic_string_view<CharT>, Src>
	inline CharT* copy( CharT( &dst )[N], Src&& src ) noexcept {
		return copy( dst, N, std::forward<Src>( src ) );
	}

    // Trim whitespace at both ends
    template< typename _CharT >
    inline std::basic_string_view<_CharT> trim( std::basic_string_view<_CharT> sv ) {
        while (!sv.empty() && std::isspace(sv.front())) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(sv.back()))  sv.remove_suffix(1);
        return sv;
    }

    // Trim whitespace at both ends
    template< typename _CharT >
    inline std::basic_string_view<_CharT> trim( _CharT const* src ) {
        std::basic_string_view<_CharT> sv{src};
        while (!sv.empty() && std::isspace(sv.front())) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(sv.back()))  sv.remove_suffix(1);
        return sv;
    }

    // Trim whitespace at both ends
    template< typename _CharT >
    inline std::basic_string_view<_CharT> trim( std::basic_string<_CharT> const& src ) {
        std::basic_string_view<_CharT> sv{src};
        while (!sv.empty() && std::isspace(sv.front())) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(sv.back()))  sv.remove_suffix(1);
        return sv;
    }

    template< typename _CharT>
    inline std::vector<std::basic_string_view<_CharT>> split( std::basic_string<_CharT> const& s, _CharT delim ) {
        std::vector<std::basic_string_view<_CharT>> parts;
		std::basic_string_view<_CharT> sv{ s };
        while( 1 ) {
            auto pos = sv.find( delim );
            if( pos == std::basic_string_view<_CharT>::npos ) {
                parts.push_back( sv );
                break;
            } else {
                parts.push_back( sv.substr( 0, pos ) );
                sv.remove_prefix( pos + 1 );
            }
        }
        return parts;
    }

    template< typename _CharT>
    inline std::vector<std::basic_string_view<_CharT>> split( std::basic_string_view<_CharT> sv, _CharT delim ) {
        std::vector<std::basic_string_view<_CharT>> parts;
        while( 1 ) {
            auto pos = sv.find( delim );
            if( pos == std::basic_string_view<_CharT>::npos ) {
                parts.push_back( sv );
                break;
            } else {
                parts.push_back( sv.substr( 0, pos ) );
                sv.remove_prefix( pos + 1 );
            }
        }
        return parts;
    }

#ifdef _TCHAR_DEFINED
        #ifdef  UNICODE 
            using tstring = std::wstring;
            #define to_tstring to_wstring
        #else // def UNICODE
            using tstring = std::string;
            #define to_tstring to_string
        #endif // def UNICODE
        // to make tstring work, we need identity conversions
        constexpr std::string to_string( std::string const& s ) { return s; };
        constexpr std::wstring to_wstring( std::wstring const& s ) { return s; };
        #define TSTRING( a ) tstring( _T( a ) )
    #endif // def _TCHAR_DEFINED
}; // namespace VWBUtil
