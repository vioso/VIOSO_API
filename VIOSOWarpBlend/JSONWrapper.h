#pragma once
#include <string>
#include <map>
#include <vector>
#include <istream>
#include <sstream>
#include <utility>

class JSONWrapper
{
public:
	enum class NODE_TYPE
	{
		EMPTY,
		BOOL,
		UINT,
		INT,
		FLOAT,
		OBJECT,
		LIST,
		STRING,
		END
	};
	
	static const std::string whitespaces;

	struct Value
	{
		NODE_TYPE type;

		typedef std::vector<Value> List;
		typedef std::map< std::string, Value > Object;

		union {
			Object object;
			List lst;
			std::string str;
			bool boolean;
			uint64_t uint;
			int64_t integer;
			double floating;
		};

		Value() : type(NODE_TYPE::EMPTY), floating( 0.0 ) {}
		Value( Object const& obj ) : type( NODE_TYPE::OBJECT ) 
		{
			new(&object) Object( obj ); 
		}
		Value( std::vector<Value> const& list ) : type( NODE_TYPE::LIST ) 
		{ 
			new(&lst) List( list ); 
		}
		Value( std::string const& s ) : type( NODE_TYPE::STRING )
		{
			new(&str) std::basic_string( s ); 
		}
		Value( bool b ) : type( NODE_TYPE::BOOL ), boolean( b ) {}
		Value( int64_t i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Value( uint64_t i ) : type( NODE_TYPE::UINT ), uint( i ) {}
		Value( double d ) : type( NODE_TYPE::FLOAT ), floating( d ) {}
		Value( Value const& other )  noexcept : type(other.type)
		{
			// we need to deep copy all the data
			switch (type)
			{
			case NODE_TYPE::OBJECT:
				new( &object ) Object( other.object );
				break;
			case NODE_TYPE::LIST:
				new( &lst ) List( other.lst );
				break;
			case NODE_TYPE::STRING:
				new( &str ) std::basic_string( other.str );
				break;
			default:
				floating = other.floating;
			};
		}

		Value(Value&& other)  noexcept : type(other.type)
		{
			// no need to deep copy, we just move
			switch (type)
			{
			case NODE_TYPE::OBJECT:
				memcpy( &object, &other.object, sizeof( object ) );
				break;
			case NODE_TYPE::LIST:
				memcpy( &lst, &other.lst, sizeof(lst) );
				break;
			case NODE_TYPE::STRING:
				memcpy( &str, &other.str, sizeof(str) );
				break;
			default:
				floating = other.floating;
			};
			other.type = NODE_TYPE::EMPTY; // prevent destruction of complex members
		}

		~Value()  noexcept
		{
			// clean up
			switch (type)
			{
			case NODE_TYPE::OBJECT:
				object.~Object();
				break;
			case NODE_TYPE::LIST:
				lst.~vector<Value>();
				break;
			case NODE_TYPE::STRING:
				str.~basic_string();
				break;
			}
		}

		Value& operator=(Value&& other) noexcept
		{ 
			std::swap(*this, other); 
			return *this; 
		};
		Value& operator=(Value const& other)  noexcept
		{
			// destruct me
			this->~Value();
			type = other.type;

			// we need to deep copy all the data
			switch (type)
			{
			case NODE_TYPE::OBJECT:
				new( &object ) Object( other.object );
				break;
			case NODE_TYPE::LIST:
				new( &lst ) List( other.lst );
				break;
			case NODE_TYPE::STRING:
				new( &str ) std::string( other.str );
				break;
			default:
				floating = other.floating;
			};
			return *this;
		};

		Value const& operator[]( std::string tag ) const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				if( auto v = object.find(tag); v != object.end() )
					return v->second;
				else
					return Value();
			}
			throw std::invalid_argument( "node type need to be object" );
			return *this;
		}

		Value& operator[]( std::string tag )
		{
			if( NODE_TYPE::OBJECT == type )
			{
				if( auto v = object.find(tag); v != object.end() )
					return v->second;
				throw std::overflow_error("object member not found");
			}
			throw std::invalid_argument( "node type need to be object" );
			return *this;
		}

		Value const& operator[](size_t index) const
		{
			if (NODE_TYPE::LIST == type)
			{
				if (lst.size() > index)
					return lst[index];
				else
					return Value();
			}
			throw std::invalid_argument("node type need to be list");
			return *this;
		}

		Value const operator[](size_t index)
		{
			if (NODE_TYPE::LIST == type)
			{
				if (lst.size() > index)
					return lst[index];
				throw std::overflow_error("list index out of bounds");
			}
			throw std::invalid_argument("node type need to be list");
			return *this;
		}

		operator bool() const
		{
			if( NODE_TYPE::BOOL == type )
				return boolean;
			else
			{
				if( isIntegral() )
				{
					return uint != 0;
				}
			}
			return false;
		}

		bool isIntegral() const
		{ 
			return NODE_TYPE::STRING <= type && type < NODE_TYPE::END;
		}

		bool changeTypeTo( NODE_TYPE newType )
		{
			try {
				switch( type )
				{
				case NODE_TYPE::STRING:
				{
					if( type == NODE_TYPE::STRING )
						break;

					std::string tmp;
					std::swap( tmp, str );
					str.~basic_string();
					
					switch( newType )
					{
					case NODE_TYPE::BOOL:
					{
						const char* s( "TRUE" );
						boolean = std::equal( str.begin(), str.end(), s, s + 5,
											  []( char const& c1, char const& c2 )
						{
							return c1 == c2 || toupper( c1 ) == toupper( c2 );
						} );
						type = NODE_TYPE::BOOL;
						break;
					}
					case NODE_TYPE::UINT:
						uint = std::stoull( str );
						break;
					case NODE_TYPE::INT:
						integer = std::stoll( str );
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = std::stod( str );
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
					delete &str;
				}
				case NODE_TYPE::BOOL:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						new( &str ) std::basic_string( boolean ? "true" : "false" );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						break;
					case NODE_TYPE::UINT:
						uint = boolean ? 1 : 0;
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						integer = boolean ? 1 : 0;
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = boolean ? 1 : 0;
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::UINT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						new( &str ) std::basic_string( std::to_string( uint ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != uint;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						break;
					case NODE_TYPE::INT:
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = (double)uint;
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::INT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						new(&str) std::basic_string( std::to_string( integer ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != integer;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						break;
					case NODE_TYPE::FLOAT:
						floating = double(integer);
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::FLOAT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						new( &str ) std::basic_string( std::to_string( floating ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != floating;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						uint = (uint64_t)floating;
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						integer = (int64_t)floating;
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				default:
					throw std::invalid_argument( "invald type to convert from" );
				};
			}
			catch( std::exception& e )
			{
				( e );
				return false;
			}
			return true;
		};

		size_t size() const
		{
			if( NODE_TYPE::LIST == type )
				return lst.size();
			throw std::invalid_argument( "invald type to convert to" );
		}

		std::vector<std::string> names() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				std::vector<std::string> vs;
				for( auto const& p : object )
					vs.push_back( p.first );
				
			}
			throw std::invalid_argument( "invald type to convert to" );
			return std::vector<std::string>();
		}

		std::string to_string( int indent = 0) const
		{
			std::string ss;
			std::ostringstream s(ss);
			try {
				switch( type )
				{
				case NODE_TYPE::OBJECT:
					for( int i = 0; i != indent; i++ )
						s << " ";
					s << "{\n";
					for( auto p = object.begin(); p != object.end(); p++ )
					{
						if( p != object.begin() )
							s << ",\n";
						for( int i = 0; i != indent; i++ )
							s << " ";
						s << "  \"" << p->first << "\" : " << p->second.to_string( indent + 2 );
					}
					for( int i = 0; i != indent; i++ )
						s << " ";
					s << "}";
					break;
				case NODE_TYPE::LIST:
					for( int i = 0; i != indent; i++ )
						s << " ";
					s << "[";
					for( auto p = lst.begin(); p != lst.end(); p++ )
					{
						if( p != lst.begin() )
							s << ", ";
						s << p->to_string( indent );
					}
					s << " ]";
					break;
				case NODE_TYPE::STRING:
					s << "\"" << str << "\"";
					break;
				case NODE_TYPE::BOOL:
					s << boolean ? "true" : "false";
					break;
				case NODE_TYPE::UINT:
					s << uint;
					break;
				case NODE_TYPE::INT:
					s << integer;
					break;
				case NODE_TYPE::FLOAT:
					s << floating;
					break;
				default:
					throw std::invalid_argument( "invald type" );
				};
			}
			catch( std::exception& e )
			{
				( e );
			}
			return ss;
		};

		// parses a string into an object
		bool parse();
		bool parse(std::istream& ss);
	};

protected:
	Value root;
	bool prettyPrint;

public:
	JSONWrapper();
	JSONWrapper( std::istream&& s );
	JSONWrapper( std::string s );
	virtual ~JSONWrapper();
	std::string to_string() const;
};

//class JSONRPC : public JSONWrapper
//{
//public:
//	JSONWrapper();
//	JSONWrapper(std::string s);
//	JSONWrapper(std::istream s);
//	virtual ~JSONWrapper();
//
//protected:
//	int id;
//	std::string method;
//};