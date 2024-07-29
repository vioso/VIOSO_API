#pragma once
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "../Include/StringConversions.h"

namespace VWBUtil
{
    template < class string_elem_type = char >
    class ParamMap {
    public:
        using string_type = std::basic_string<string_elem_type>;
        using array_type = std::vector<string_type>;
        using map_type = std::map<string_type, array_type >;
        using value_type = std::map<string_type, array_type>::value_type;

        /// Attributes
        typedef struct AttributeProps {
            int numArguments; // number of arguments; 0 is a flag, it creates a "true" argument; or more to force a number of arguments. Set to -1 for "1 or more".
            bool mandatory; // attribute must be present
            array_type def; // default, set empty to keep no arguments, if set it parsed like a command line would be, if no arguments are passed
        } Attribute;

        /// Commands translate to attributes
        typedef struct CommandProps {
            string_type attribute;
            array_type arguments;
        } Command;

        // initialize parser
        typedef struct Init {
            bool allowUnInit; // set to true to allow parameters without a corresponding attribute
            string_elem_type defaultCommand; // is used for first argument, if this is no command or attribute
            typedef std::map< string_type, AttributeProps > AttributeMap;
            AttributeMap attributes;
            typedef std::map< string_elem_type, CommandProps > CommandMap;
            CommandMap commands;
        } Init;

    private:
        map_type params;

        array_type makeSV( int argc, string_elem_type* argv[] ) {
            return array_type( argv + 1, argv + argc );
        }

        array_type makeSV( string_elem_type const* s ) {
            array_type ret;
            std::basic_istringstream<string_elem_type> ss( s );
            string_type param;
            while( 1 )
            {
                param.clear();
                if( ss.eof() || ss.bad() )
                    break;
                if( std::filesystem::path::preferred_separator == '\\' )
                    ss >> quoted( param, string_elem_type('"'), string_elem_type('^') );
                else
                    ss >> quoted( param );
                if( param.empty() )
                    break;
                ret.push_back( param );
            }
            return ret;
        }
    public:

        /// constructors
        /// @param init a initializer @see Params::Init
        /// @param sv a list of strings
        ParamMap( array_type const& sv, Init const& init = Init{ true } ) {
            // unwrap init
            using namespace std;
            auto arguments = params.end();
            for( auto const& p : sv )
            {
                if( '-' == p[0] ) // set new attribute
                {
                    if( p.size() < 2 )
                        throw invalid_argument( "A single - is not allowed. Must be followed by another - to specify an attribute or a single character, to specify a command." );
                    if( '-' == p[1] )
                    {
                        string_type at = p.substr( 2 );
                        if( !init.allowUnInit && init.attributes.end() == init.attributes.find( at ) )
                            throw invalid_argument( string( "Unknown attribute --" ) + to_string( at ) );
                        arguments = params.emplace( value_type{ at, {} } ).first;
                    }
                    else
                    {

                        for( size_t i = 1; i < p.size(); i++ )
                        {
                            auto ii = init.commands.find( p[i] );
                            if( init.commands.end() == ii )
                                throw invalid_argument( string( "Unknown command or flag-" ) + char( p[i] ) );
                            arguments = params.emplace( value_type{ ii->second.attribute, {} } ).first;
                            // override old arguments
                            arguments->second = ii->second.arguments;
                            if( auto iii = init.attributes.find( ii->second.attribute ); iii != init.attributes.end() && 0 != iii->second.numArguments && p.size() > i + 1 )
                            {
                                arguments->second.emplace_back( p.substr( i + 1 ) );
                                break;
                            }
                        }
                    }
                }
                else if( arguments != params.end() )
                { // this is an attribute
                    if( arguments->second.empty() )
                        arguments->second.emplace_back( p );
                    else if( auto ii = init.attributes.find( arguments->first ); init.attributes.end() == ii || -1 == ii->second.numArguments || int( arguments->second.size() ) < ii->second.numArguments )
                        arguments->second.emplace_back( p );
                    else
                        throw invalid_argument( string( "Too many arguments for attribute --" ) + to_string( arguments->first ) + "." );
                }
                else
                {
                    if( init.defaultCommand )
                    {
                        auto defIt = init.commands.find( init.defaultCommand );
                        if( defIt == init.commands.end() )
                            throw invalid_argument( string( 1, char( init.defaultCommand ) ) + " can't be used as default command, because it is not a known command." );
                        arguments = params.emplace( value_type{ defIt->second.attribute, defIt->second.arguments } ).first;
                        arguments->second.emplace_back( p );
                    }
                    else
                        throw invalid_argument( "Argument without attribute specified. Use a command or attribute specifier before." );
                }
            }

            // set defaults or true for isFlag values, if nothing is set; complain about missing attributes
            for( auto& attr : params )
            {
                if( attr.second.empty() )
                {
                    if( auto ii = init.attributes.find( attr.first ); init.attributes.end() != ii )
                    {
                        if( !ii->second.def.empty() )
                        {
                            attr.second = ii->second.def;
                        }
                        else if( 0 == ii->second.numArguments )
                        {
                            attr.second.emplace_back( string_type( 1, 1 ) );
                        }
                        else if( ii->second.mandatory )
                            throw invalid_argument( string( "Argument --" ) + to_string( attr.first ) + " must have some attribute." );
                        else
                            throw invalid_argument( string( "Too few arguments for attribute --" ) + to_string( attr.first ) );
                    }
                    else if( !init.allowUnInit )
                        throw invalid_argument( string( "Unknown attribute --" ) + to_string( attr.first ) );
                }
                else if( attr.second.size() > 1 ) // check if multi
                {
                    if( auto ii = init.attributes.find( attr.first ); init.attributes.end() != ii && -1 != ii->second.numArguments && int( arguments->second.size() ) != ii->second.numArguments )
                        throw invalid_argument( string( "Wrong number of arguments for attribute --" ) + to_string( attr.first ) );
                }
            }

            // check for missing arguments
            for( auto const& attr : init.attributes )
            {
                if( attr.second.mandatory && params.end() == params.find( attr.first ) )
                {
                    if( !attr.second.def.empty() )
                        params.emplace( attr.first, attr.second.def );
                    else
                        throw invalid_argument( string( "Argument " ) + to_string( attr.first ) + " is mandatory." );
                }
            }
        }

        /// Construct with a string from winMain().
        /// Quoted sequences by " (double quote) are treated as single words, quotes are removed. Quotes can be escaped by '\' (back-slash) to be ignored. The escape char is removed.
        /// @param init a initializer @see Params::Init
        /// @param s a string split by white spaces, SPACE or TAB
        ParamMap( string_elem_type* s, Init const& init = Init{ true } ) : ParamMap( makeSV( s ), init ) {}
        ParamMap( string_elem_type const* s, Init const& init = Init{ true } ) : ParamMap( makeSV( s ), init ) {}
        ParamMap( string_type const& s, Init const& init = Init{ true } ) : ParamMap( makeSV( s.c_str() ), init ) {}

        /// Construct with arguments from main()
        /// @param init a initializer @see Params::Init
        /// @param argc the argument count
        /// @param argv the argument vector (like in main() call, argv[0] is invoked command, argv[1] the first parameter
        ParamMap( int argc, string_elem_type* argv[], Init const& init = Init{ true } ) : ParamMap( makeSV( argc, argv ), init ) {}

        /// We use move semantic to allow RAII use
        ParamMap() {}
        ParamMap( ParamMap const& ) = delete;
        ParamMap( ParamMap&& other ) noexcept : params( std::move( other.params ) ) {}
        ParamMap& operator=( ParamMap const& ) = delete;
        ParamMap& operator=( ParamMap&& ) = default;

        bool is( string_type const& name ) const {
            if( auto it = params.find( name ); it != params.end() && !it->second.empty() && !it->second.front().empty() )
                return 0 != it->second.front().front();
            return false;
        }
        bool has( string_type const& name ) const {
            if( auto it = params.find( name ); it != params.end() )
                return true;
            return false;
        }

        array_type const& operator[]( string_type const& name ) const {
            static const array_type _empty;
            if( auto it = params.find( name ); it != params.end() )
                return it->second;
            return _empty;
        }
    };
}; // namespace VWBUtil
