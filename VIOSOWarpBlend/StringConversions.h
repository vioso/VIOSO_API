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
#include <locale.h>
namespace VWBUtil
{
    /// helper for secure versions
    #ifdef WIN32
        template< size_t size >
        size_t _wcrtomb( char( &c )[size], wchar_t wc, mbstate_t* state ) { size_t r; if( 0 == wcrtomb_s( &r, c, size, wc, state ) ) return r; else return size_t(-1); }
    #else
        using _wcrtomb = std::wcrtomb;
    #endif

        #if 0 // alternative implementation
        /// Helper for auto locale. 
        /// As we want wstring to string and vice versa conversions to work, we must set system's user preferred locale "" (empty string).
        /// Initially (at program start) the locale is set to std::locale::classic equivalent to "C".
        /// This is used globally in the process. It will affect numbers, date/time and so on.
        inline void _initLocale() // inline will not expand but use a naked call
        {
            // Note the static here, it is called just once
            static auto __intern = std::locale::global( std::locale( "" ) );
        }
        #endif

    #ifndef VWB_NO_AUTOLOCALE
        /// Helper for auto locale. 
        /// As we want wstring to string and vice versa conversions to work, we must set system's user preferred locale "" (empty string).
        /// We only set the locale for LC_CTYPE. This affects character set only.
        /// Initially (at program start) the locale is set to minimal "C".
        /// If you want some other locale set, use setLocale before calling any wstring/string conversion.
        /// @return reference to current locale identifiers
        inline std::string* _initLocale()
        {
            // Note the static here, we use string to force an instant deep copy of the returned value.
            // Both calls are guaranteed to return not nullptr.
            static std::string curr[2] = { 
                setlocale( LC_CTYPE, nullptr ),
                setlocale( LC_CTYPE, "" ) };
            return curr;
        }

        /// set the locale and keep track of
        /// Note: This call is not thread-safe.
        /// @param cat the category, one of the LC_xxx macros
        /// @param loc system specific locale identifier, can be "" for user preferred, "C" for minimal
        /// @return pointer to a narrow null-terminated string identifying the C locale after applying the changes, if any, or null pointer on failure. 
        inline char const* setLocale( int cat = LC_ALL, const char* loc = "" )
        {
            std::string* ll = _initLocale();
            const char* nl = setlocale( cat, loc );
            if( nl ) // we test, if the call succeeded, we want to keep track of the current locale
            {
                ll[1] = nl; // change on success
                return ll[1].c_str(); // we return new locale identifier
            }
            return nullptr; // behave like a normal call and return nullptr on failure
        }
        /// gets the last set locale by setLocale function
        /// @return pointer to a narrow null-terminated string identifying the C locale currently active when set through setLocale
        inline char const* getLocale()
        {
            return _initLocale()[1].c_str();
        }

        /// gets the initlal locale before static initialization. This is usually "C".
        /// @return pointer to a narrow null-terminated string identifying the initlal locale before static initialization
        inline char const* getInitialLocale()
        {
            return _initLocale()[0].c_str(); // OK, as the initial call will not return nullptr, thus the string is not empty.
        }
    #else  // ndef VWB_NO_AUTOLOCALE
        inline constexpr void _initLocale() {}
    #endif // ndef VWB_NO_AUTOLOCALE

    ///  converts a locale string to wstring
    inline std::wstring to_wstring( std::string const& s )
    {
        _initLocale();
        auto sz = s.size();
        std::wstring ws; ws.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto c = s.data(), cE = c + sz; c < cE; )
        {
            if( !( 0x80 & *c ) ) 
            {
                ws.push_back( std::wstring::value_type( *(c++) ) );
            }
            else
            {
                std::wstring::value_type cv = 0;
                std::mbstate_t state{};
                size_t a = std::mbrtowc( &cv, c, cE - c, &state );
                if( 4 < a )
                { // error or inclomplete
                    c++; // try next
                }
                else
                {
                    ws.push_back( cv );
                    c += a;
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
        // try to be quick, if all chars are simple ASCII
        for( auto c = (char*)u8s.data(), cE = c + sz; c < cE; )
        {
            if( !( 0x80 & *c ) )
            {
                ws.push_back( std::wstring::value_type( *( c++ ) ) );
            }
            else
            {
                char16_t wc = 0;
                std::mbstate_t state{};
                size_t a = std::mbrtoc16( &wc, c, cE - c, &state );
                if( 4 < a )
                { // error or inclomplete
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

    /// converts a wstring to string on current locale
    inline std::string to_string( std::wstring const& ws )
    {
        _initLocale();
        auto sz = ws.size();
        std::string s; s.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto wc : ws )
        {
            if( 0x80 > wc )
            {
                s.push_back( std::string::value_type( wc ) );
            }
            else
            {
                char c[4];
                std::mbstate_t state{};
                size_t a = _wcrtomb( c, wc, &state );
                if( 4 >= a )
                    s.append( c, c + a );
            }
        }
        return s;
    }

    /// converts a wstring to u8string
    inline std::u8string to_u8string( std::wstring const& ws )
    {
        auto sz = ws.size();
        std::u8string u; u.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto wc : ws )
        {
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

    /// converts a utf-8 string to locale codepage
    inline std::string to_string( std::u8string const& u8s )
    {
        auto sz = u8s.size();
        std::string s; s.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto u8c = ( char* )u8s.data(), cE = u8c + sz; u8c < cE; )
        {
            if( !( 0x80 & *u8c ) )
            {
                s.push_back( char( *(u8c++) ) );
            }
            else
            {
                char16_t uc;
                std::mbstate_t state{};
                size_t a = std::mbrtoc16( &uc, u8c, u8c - cE, &state );
                if( 4 >= a )
                {
                    u8c += a;
                    char c[4];
                    state = std::mbstate_t{};
                    a = _wcrtomb( c, uc, &state );
                    if( 4 >= a )
                        s.append( c, c + a );
                }
                else
                    u8c++;
            }
        }

        return s;
    }

    ///  converts a locale string to utf-8 string
    inline std::u8string to_u8string( std::string const& s )
    {
        auto sz = s.size();
        std::u8string u8s; u8s.reserve( sz );
        // try to be quick, if all chars are simple ASCII
        for( auto c = s.data(), cE = c + sz; c < cE; )
        {
            if( !( 0x80 & *c ) )
            {
                u8s.push_back( char8_t( *( c++ ) ) );
            }
            else
            {
                wchar_t uc;
                std::mbstate_t state{};
                int a = std::mbtowc( &uc, c, c - cE );
                if( 0 < a )
                {
                    c += a;
                    char u8c[4];
                    state = std::mbstate_t{};
                    size_t b = std::c16rtomb( u8c, uc, &state );
                    if( 4 >= b )
                        u8s.append( u8c, u8c + b );
                }
                else
                    c++;
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
        #define TSTRING( a ) tstring( _T( a ) )
    #endif // def _TCHAR_DEFINED
}; // namespace VWBUtil
