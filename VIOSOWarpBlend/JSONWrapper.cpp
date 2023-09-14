#include "JSONWrapper.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

const std::string JSONWrapper::whitespaces{ " \t\n\r" };

JSONWrapper::JSONWrapper()
	: prettyPrint(false)
{
}

JSONWrapper::JSONWrapper( std::istream&& s )
	: JSONWrapper()
{
	root.parse( s );
}

JSONWrapper::JSONWrapper( std::string s )
	: JSONWrapper( std::istringstream(s) )
{
}


JSONWrapper::~JSONWrapper()
{
}

std::string JSONWrapper::to_string() const
{
	return root.to_string();
}

bool JSONWrapper::Value::parse()
{
	// check if string
	if( type == NODE_TYPE::STRING )
	{
		std::istringstream iss( str );
		return parse( iss );
	}
	return false;
}

bool JSONWrapper::Value::parse( std::istream& ss )
{
	this->~Value();
	if( ss.good() && !ss.eof() )
	{
		char c = 0;
		ss >> std::skipws >> c;
		if( '{' == c )
		{
			// an object
			type = NODE_TYPE::OBJECT;
			new(&object) Object();

			while( 1 )
			{
				ss >> c;
				if( '"' == c )
				{
					// rewind to catch the quote
					ss.seekg( -1, std::ios::cur );
					std::string name;
					ss >> std::quoted( name ) >> c;
					if( ':' == c )
					{
						Value v;
						if( v.parse( ss ) )
							object.emplace( name, std::move(v) );
						else
							return false;
					}
					// try read a comma
					ss >> c;
					if( ',' != c )
						break;
				}
				else
					return false; // something wrong with syntax
			}
			// there must be a closing brace
			if( c == '}' )
				return true;
		}
		else if( c == '[' )
		{
			// a list of values
			type = NODE_TYPE::LIST;
			new(&object) List();

			while( 1 )
			{
				Value v;
				if( v.parse( ss ) )
					lst.emplace_back( v );
				else
					return false;
				// try read a comma
				ss >> c;
				if( ',' != ss.peek() )
				{
					break;
				}

			}
			if( c == ']' )
				return true;
		}
		else
		{
			// a pure value
			// we must rewind last get, as it already belongs to the value
			ss.seekg( -1, std::ios::cur );
			std::string value;
			bool hasQuote = '"' == ss.peek();
			ss >> std::quoted( value );

			if( !value.empty() )
			{
				if( !hasQuote )
				{
					// if there was a comma or other delimiter right after, it has been read too, sw we need to rewind everything that is not a ascii letter or a number or a dot
					while( !value.empty() )
					{
						if( '0' <= value.back() && '9' >= value.back() ||
							'a' <= value.back() && 'z' >= value.back() ||
							'A' <= value.back() && 'Z' >= value.back() ||
							'.' == value.back() )
							break;
						value.pop_back();
					}
					if( char cmp[]{ "true" }; std::equal( value.begin(), value.end(), cmp, cmp + 4, []( char a, char b ) { return std::tolower( a ) == std::tolower( b ); } ) )
					{
						type = NODE_TYPE::BOOL;
						boolean = true;
						return true;
					}
					else if( char cmp[]{ "false" }; std::equal( value.begin(), value.end(), cmp, cmp + 5, []( char a, char b ) { return std::tolower( a ) == std::tolower( b ); } ) )
					{
						type = NODE_TYPE::BOOL;
						boolean = false;
						return true;
					}
					if( char cmp[]{ "null" }; std::equal( value.begin(), value.end(), cmp, cmp + 4, []( char a, char b ) { return std::tolower( a ) == std::tolower( b ); } ) )
					{
						type = NODE_TYPE::EMPTY;
						return true;
					}
					else
					{ // number
						try
						{
							double d = std::stod( value );
							uint64_t u = std::stoull( value );
							uint64_t i = std::stoll( value );
							if( d >= 0 && uint64_t( d ) == u )
							{
								type = NODE_TYPE::UINT;
								uint = u;
							}
							else if( int64_t( d ) == i )
							{
								type = NODE_TYPE::INT;
								integer = i;
							}
							else
							{
								type = NODE_TYPE::FLOAT;
								floating = d;
							}
							return true;
						}
						catch( std::exception& )
						{
							return false;
						}
					}
				}
				else
				{
					new(&str) std::basic_string( value );
					type = NODE_TYPE::STRING;
					return true;
				}
			}
		}
	}
	return false;
}
