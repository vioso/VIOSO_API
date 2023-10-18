#pragma once
#include <string>
#include <map>
#include <vector>
#include <istream>
#include <sstream>
#include <iomanip>
#include <utility>

#ifdef WIN32
#include <windows.h>
/// converts a utf-8 string to locale codepage
/// note: MultiByteToWideChar and WideCharToMultiByte will handle the string with or without terminating '\0', the '\0' will be present in output, if set in input.
inline std::string to_string( std::u8string const& u )
{
	std::wstring ws;
	auto sz = ::MultiByteToWideChar( CP_UTF8, 0, ( const char* )u.data(), (int)u.size(), NULL, 0 );
	ws.resize( sz, '\0' );
	::MultiByteToWideChar( CP_UTF8, 0, ( const char* )u.data(), (int)u.size(), ws.data(), (int)ws.size() );
	sz = ::WideCharToMultiByte( CP_ACP, 0, ws.data(), ( int )ws.size(), nullptr, 0, nullptr, nullptr );
	std::string s( sz, '\0' );
	::WideCharToMultiByte( CP_ACP, 0, ws.data(), (int)ws.size(), s.data(), (int)s.size(), nullptr, nullptr );
	return s;
}

///  converts a locale string to utf-8 string
inline std::u8string to_u8string( std::string const& s )
{
	std::wstring ws;
	auto sz = ::MultiByteToWideChar( CP_UTF8, 0, s.data(), ( int )s.size(), NULL, 0 );
	ws.resize( sz, '\0' );
	::MultiByteToWideChar( CP_UTF8, 0, s.data(), ( int )s.size(), ws.data(), ( int )ws.size() );
	sz = ::WideCharToMultiByte( CP_UTF8, 0, ws.data(), ( int )ws.size(), nullptr, 0, nullptr, nullptr );
	std::u8string u( sz, '\0' );
	::WideCharToMultiByte( CP_UTF8, 0, ws.data(), ( int )ws.size(), ( char* )u.data(), ( int )u.size(), nullptr, nullptr );
	return u;
}
#endif // def WIN32

template< typename _Elem >
struct _quoted {
	std::basic_string<_Elem>& _Str;
	_Elem const* _Quotes;
	_Elem const* _Delims;
	_Elem const _Escape;
	_quoted( std::basic_string<_Elem>& s, _Elem const* quotes, _Elem const* delims, _Elem const& escape ) : _Str( s ), _Quotes( quotes ), _Delims( delims ), _Escape( escape ) {}
};
template< typename _Elem >
static _quoted<_Elem> quoted( std::basic_string<_Elem>& s ) { return _quoted<_Elem>( s, ( _Elem const* )"\"'", ( _Elem const* )", \t\n\r", '\\' ); }
template< typename _Elem >
static _quoted<_Elem> quoted( std::basic_string<_Elem>& s, _Elem const* quotes ) { return _quoted<_Elem>( s, quotes, ( _Elem const* )", \t\n\r", '\\' ); }
template< typename _Elem >
static _quoted<_Elem> quoted( std::basic_string<_Elem>& s, _Elem const* quotes, _Elem const* delims ) { return _quoted<_Elem>( s, quotes, delims, '\\' ); }
template< typename _Elem >
static _quoted<_Elem> quoted( std::basic_string<_Elem>& s, _Elem const* quotes, _Elem const* delims, _Elem const& escape ) { return _quoted<_Elem>( s, quotes, delims, escape ); }

template <class _Elem, class _Traits > std::basic_istream<_Elem, _Traits>& operator>>( std::basic_istream<_Elem, _Traits>& _Istr, const _quoted<_Elem>& _Manip )
{
	std::ios_base::iostate _State = std::ios_base::goodbit;
	const typename std::basic_istream<_Elem, _Traits>::sentry _Ok( _Istr );
	if( _Manip._Quotes == nullptr || _Manip._Delims == nullptr || _Manip._Delims[0] == 0 || _Manip._Quotes[0] == 0 )
		throw( std::invalid_argument( "quotes and delimiters must be not null or empty" ) );
	if( _Ok )
	{ // state okay, extract characters
		try {
			const auto _Buf = _Istr.rdbuf();
			auto _Meta = _Buf->sgetc();
			auto& _Str = _Manip._Str;

			_Str.clear();
			auto checkDelim = []( decltype( _Meta ) const& c, _Elem const* delims ) -> _Elem {
				do {
					if( _Traits::eq_int_type( c, *delims ) )
						return *delims;
				} while( *( ++delims ) );
				return 0; };
			auto quote = checkDelim( _Meta, _Manip._Quotes );
			if( !quote ) { // no leading delimiter
				for( ;;) {
					if( checkDelim( _Meta, _Manip._Delims ) ) { // found trailing delimiter
						break;
					}

					_Str.push_back( _Traits::to_char_type( _Meta ) );

					_Meta = _Buf->snextc();

					if( _Traits::eq_int_type( _Traits::eof(), _Meta ) ) { // no trailing delimiter; fail
						_State = std::ios_base::eofbit | std::ios_base::failbit;
						break;
					}
				}
			}
			else
			{
				const auto _Escape = _Traits::to_int_type( _Manip._Escape );
				for( ;;)
				{
					_Meta = _Buf->snextc();
					if( _Traits::eq_int_type( _Traits::eof(), _Meta ) ) { // no trailing delimiter; fail
						_State = std::ios_base::eofbit | std::ios_base::failbit;
						break;
					}
					else if( _Traits::eq_int_type( _Meta, _Escape ) ) { // escape; read next character literally
						_Meta = _Buf->snextc();
						if( _Traits::eq_int_type( _Traits::eof(), _Meta ) ) { // bad escape; fail
							_State = std::ios_base::eofbit | std::ios_base::failbit;
							break;
						}
						else if( !_Traits::eq_int_type( _Meta, quote ) ) // if not quote, keep the escape
							_Str.push_back( _Traits::to_char_type( _Escape ) );
					}
					else if( _Traits::eq_int_type( _Meta, quote ) ) { // found trailing delimiter
						if( _Traits::eq_int_type( _Traits::eof(), _Buf->sbumpc() ) ) { // consume trailing delimiter
							_State = std::ios_base::eofbit;
						}

						break;
					}

					_Str.push_back( _Traits::to_char_type( _Meta ) );
				}
			}
		}
		catch( ... ) {
			_Istr.setstate( std::ios_base::badbit, true );
		}
	}
	_Istr.setstate( _State );
	return _Istr;
}

class JSON
{
public:
	enum class NODE_TYPE
	{
		EMPTY,
		OBJECT,
		LIST,
		STRING,
		BOOL,
		UINT,
		INT,
		FLOAT,
		END
	};
	
	constexpr static std::string whitespaces = " \t\n\r";

	typedef std::basic_ostringstream<std::u8string::value_type> ou8stringstream;
	typedef std::basic_istringstream<std::u8string::value_type> iu8stringstream;
	typedef std::basic_istream<std::u8string::value_type> iu8stream;
	typedef std::basic_ostream<std::u8string::value_type> ou8stream;

	struct Value
	{
		NODE_TYPE type;

		typedef std::vector<Value> List;
		typedef std::map< std::u8string, Value > Object;
		union {
			Object object;
			List lst;
			std::u8string str;
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
		Value( std::u8string const& s ) : type( NODE_TYPE::STRING )
		{
			new( &str ) std::u8string( s );
		}
		Value( char8_t const* s ) : type( NODE_TYPE::STRING )
		{
			new( &str ) std::u8string( s );
		}
		Value( std::string const& s ) : type( NODE_TYPE::STRING )
		{
			new( &str ) std::u8string( to_u8string(s) );
		}
		Value( char const* s ) : type( NODE_TYPE::STRING )
		{
			new( &str ) std::u8string( to_u8string( std::string( s ) ) );
		}
		Value( bool b ) : type( NODE_TYPE::BOOL ), boolean( b ) {}
		Value( int i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Value( long i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Value( int64_t i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Value( unsigned int i ) : type( NODE_TYPE::UINT ), uint( i ) {}
		Value( unsigned long i ) : type( NODE_TYPE::UINT ), uint( i ) {}
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

		Value(Value&& other) noexcept : type(other.type)
		{
			// no need to deep copy, we just move
			switch (type)
			{
			case NODE_TYPE::OBJECT:
				new( &object ) Object( std::move( other.object ) );
				break;
			case NODE_TYPE::LIST:
				new( &lst ) List( std::move( other.lst ) );
//				memcpy( &lst, &other.lst, sizeof(lst) );
				break;
			case NODE_TYPE::STRING:
				new( &str ) std::u8string( std::move( other.str ) );
//				memcpy( &str, &other.str, sizeof(str) );
				break;
			default:
				floating = other.floating;
			};
			other.type = NODE_TYPE::EMPTY; // prevent destruction of complex members
		}

		~Value() noexcept
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
			type = NODE_TYPE::EMPTY;
		}

		Value& operator=(Value&& other) noexcept
		{ 
			this->~Value();
			
			switch( other.type )
			{
			case NODE_TYPE::OBJECT:
				new( &object ) Object( std::move( other.object ) );
				break;
			case NODE_TYPE::LIST:
				new( &lst ) List( std::move( other.lst ) );
				//				memcpy( &lst, &other.lst, sizeof(lst) );
				break;
			case NODE_TYPE::STRING:
				new( &str ) std::u8string( std::move( other.str ) );
				//				memcpy( &str, &other.str, sizeof(str) );
				break;
			default:
				floating = other.floating;
			};
			type = other.type;
			other.type = NODE_TYPE::EMPTY; // prevent destruction of complex members
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
				new( &str ) std::u8string( other.str );
				break;
			default:
				floating = other.floating;
			};
			return *this;
		};

		Value const& operator[]( std::u8string const& tag ) const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				if( auto v = object.find(tag); v != object.end() )
					return v->second;
				else
					throw std::invalid_argument( std::string( "element name " ) + ::to_string( tag ) + " not found" );
			}
			throw std::invalid_argument( "node type need to be object" );
			return *this;
		}
		Value const& operator[]( char8_t const* tag ) const { return operator[]( std::u8string( tag ) ); }
		//Value const& operator[]( std::string tag ) const { return operator[]( to_u8string( tag ) ); }
		//Value const& operator[]( char const* tag ) const { return operator[]( to_u8string( std::string( tag ) ) ); }

		Value& operator[]( std::u8string const& tag )
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object[tag];
			}
			throw std::invalid_argument( "node type need to be object" );
			return *this;
		}
		Value& operator[]( char8_t const* tag ) { return operator[]( std::u8string( tag ) ); }
		//Value& operator[]( std::string const& tag ) { return operator[]( to_u8string( tag ) ); }
		//Value& operator[]( char const* tag ) { return operator[]( to_u8string( std::string( tag ) ) ); }

		Value const& operator[](size_t index) const
		{
			if (NODE_TYPE::LIST == type)
			{
				if (lst.size() > index)
					return lst[index];
				else
					throw std::invalid_argument( "index out of range" );
			}
			throw std::invalid_argument("node type need to be list");
			return *this;
		}

		Value& operator[](size_t index)
		{
			if (NODE_TYPE::LIST == type)
			{
				if( lst.size() >= index )
					lst.resize( index + 1 );
				return lst[index];
			}
			throw std::invalid_argument("node type need to be list");
			return *this;
		}

		bool isIntegral() const { return NODE_TYPE::STRING < type&& type < NODE_TYPE::END; }

		bool empty() const { return NODE_TYPE::EMPTY == type; }

		bool changeTypeTo( NODE_TYPE newType )
		{
			try {
				switch( type )
				{
				case NODE_TYPE::STRING:
				{
					if( type == NODE_TYPE::STRING )
						break;

					std::u8string tmp;
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
						uint = std::stoull( *reinterpret_cast< std::string* >( &str ) );
						break;
					case NODE_TYPE::INT:
						integer = std::stoll( *reinterpret_cast< std::string* >( &str ) );
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = std::stod( *reinterpret_cast< std::string* >( &str ) );
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
						new( &str ) std::u8string( boolean ? u8"true" : u8"false" );
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
						new( &str ) std::u8string( std::_UIntegral_to_string<char8_t>( uint ) );
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
						new(&str) std::u8string( std::_Integral_to_string<char8_t>( integer ) );
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
						new( &str ) std::u8string( to_u8string( std::to_string( floating ) ) );
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

		void push_back( Value const& other )
		{
			if( NODE_TYPE::LIST == type )
				lst.push_back( other );
			else
				throw std::invalid_argument( "must be list type to push_back" );
		}
		void push_back( Value&& other )
		{
			if( NODE_TYPE::LIST == type )
				lst.push_back( other );
			else
				throw std::invalid_argument( "must be list type to push_back" );
		}
		void pop_back()
		{
			if( NODE_TYPE::LIST == type )
				lst.pop_back();
			else
				throw std::invalid_argument( "must be list type to push_back" );
		}

		void insert( List::iterator where, Value& other )
		{
			if( NODE_TYPE::LIST == type )
				lst.insert( where, other );
			else
				throw std::invalid_argument( "must be list type to push_back" );
		}

		Value& emplace_back( Value&& other = Value() )
		{
			if( NODE_TYPE::LIST == type )
				return lst.emplace_back( other );
			throw std::invalid_argument( "must be list type to emplace_back" );
		}

		Object::iterator find( std::u8string const& what )
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.find( what );
			}
			throw std::invalid_argument( "must be object type to find" );
		}
		Object::const_iterator find( std::u8string const& what ) const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.find( what );
			}
			throw std::invalid_argument( "must be object type to find" );
		}

		template< typename OTy >
		OTy::iterator begin()
		{
			throw std::invalid_argument( "must be object or list type" );
		}
		template<>
		List::iterator begin<List>()
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.begin();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::iterator begin<Object>()
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.begin();
			}
			throw std::invalid_argument( "must be object type" );
		}

		template< typename OTy >
		OTy::const_iterator begin() const
		{
			throw std::invalid_argument( "must be object or list type" );
		}
		template<>
		List::const_iterator begin<List>() const
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.begin();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::const_iterator begin<Object>() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.begin();
			}
			throw std::invalid_argument( "must be object type" );
		}

		template< typename OTy >
		OTy::iterator end()
		{
			throw std::invalid_argument( "must be list or object type " );
		}
		template<>
		List::iterator end<List>()
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.end();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::iterator end<Object>()
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.end();
			}
			throw std::invalid_argument( "must be object type " );
		}
		template< typename OTy >
		OTy::const_iterator end() const
		{
			throw std::invalid_argument( "must be list or object type " );
		}
		template<>
		List::const_iterator end<List>() const
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.end();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::const_iterator end<Object>() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.end();
			}
			throw std::invalid_argument( "must be object type " );
		}

		template< typename OTy >
		OTy::reverse_iterator rbegin()
		{
			throw std::invalid_argument( "must be object or list type" );
		}
		template<>
		List::reverse_iterator rbegin<List>()
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.rbegin();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::reverse_iterator rbegin<Object>()
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.rbegin();
			}
			throw std::invalid_argument( "must be object type" );
		}
		template< typename OTy >
		OTy::const_reverse_iterator rbegin() const
		{
			throw std::invalid_argument( "must be object or list type" );
		}
		template<>
		List::const_reverse_iterator rbegin<List>() const
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.rbegin();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::const_reverse_iterator rbegin<Object>() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.rbegin();
			}
			throw std::invalid_argument( "must be object type" );
		}

		template< typename OTy >
		OTy::reverse_iterator rend()
		{
			throw std::invalid_argument( "must be list or object type " );
		}
		template<>
		List::reverse_iterator rend<List>()
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.rend();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::reverse_iterator rend<Object>()
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.rend();
			}
			throw std::invalid_argument( "must be object type " );
		}
		template< typename OTy >
		OTy::const_reverse_iterator rend() const
		{
			throw std::invalid_argument( "must be list or object type " );
		}
		template<>
		List::const_reverse_iterator rend<List>() const
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.rend();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		Object::const_reverse_iterator rend<Object>() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.rend();
			}
			throw std::invalid_argument( "must be object type " );
		}

		template< typename OTy >
		size_t size() const
		{
			throw std::invalid_argument( "must be list or object type " );
		}
		template<>
		size_t size<List>() const
		{
			if( NODE_TYPE::LIST == type )
			{
				return lst.size();
			}
			throw std::invalid_argument( "must be list type" );
		}
		template<>
		size_t size<Object>() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				return object.size();
			}
			throw std::invalid_argument( "must be object type " );
		}

		std::vector<std::u8string> names() const
		{
			if( NODE_TYPE::OBJECT == type )
			{
				std::vector<std::u8string> vs;
				for( auto const& p : object )
					vs.push_back( p.first );

			}
			throw std::invalid_argument( "must be object type to get names" );
			return std::vector<std::u8string>();
		}

		// comparison operators
		std::partial_ordering operator<=>( Value const& other ) const {
			if( isIntegral() && other.isIntegral() )
			{
				if( type == other.type )
				{
					if( floating == other.floating )
						return std::strong_ordering::equal;
					switch( type )
					{
					case NODE_TYPE::BOOL:
						return boolean <=> other.boolean;
					case NODE_TYPE::INT:
						return integer <=> other.integer;
					case NODE_TYPE::UINT:
						return uint <=> other.uint;
					case NODE_TYPE::FLOAT:
						return floating <=> other.floating;
					}
				}
				if( floating < other.floating ) {}
			}
			return std::strong_ordering::equivalent;
		}
		// const access methods
		operator bool() const { if( isIntegral() ) return floating != 0; else throw( std::runtime_error( "wrong type" ) ); }
		operator int() const { 
			if( isIntegral() ) { 
				return type == NODE_TYPE::FLOAT ? ( int )floating : ( int )integer; } 
			else throw( std::runtime_error( "wrong type" ) );	}
		operator unsigned int() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? ( unsigned int )floating : ( unsigned int )uint;  } else throw( std::runtime_error( "wrong type" ) ); }
		//operator long const&() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? ( long )floating : ( long )integer; } else throw( std::runtime_error( "wrong type" ) ); }
		//operator unsigned long const&() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? ( unsigned long )floating : ( long )uint; } else throw( std::runtime_error( "wrong type" ) ); }
		operator int64_t() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? ( int64_t )floating : integer;  } else throw( std::runtime_error( "wrong type" ) );}
		operator uint64_t() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? ( uint64_t )floating : ( uint64_t )uint; } else throw( std::runtime_error( "wrong type" ) ); }
		operator double() const { if( isIntegral() ) { return type == NODE_TYPE::FLOAT ? floating : ( double )integer; } else throw( std::runtime_error( "wrong type" ) ); }
		operator std::u8string const& ( ) const { if( type == NODE_TYPE::STRING ) return str; else throw( std::runtime_error( "wrong type" ) ); }
	
		std::string to_string( int indent = 0) const
		{
			std::ostringstream s;
			try {
				switch( type )
				{
				case NODE_TYPE::OBJECT:
					s << "{\n";
					for( Object::const_iterator p = object.begin(); p != object.end(); p++ )
					{
						if( p != object.begin() )
							s << ",\n";
						for( int i = 0; i != indent; i++ )
							s << " ";
						s << "  \"" << encode(p->first) << "\" : " << p->second.to_string( indent + 2 );
					}
					s << "\n";
					for( int i = 0; i != indent; i++ )
						s << " ";
					s << "}";
					break;
				case NODE_TYPE::LIST:
					s << "[ ";
					for( List::const_iterator p = lst.begin(); p != lst.end(); p++ )
					{
						if( p != lst.begin() )
							s << ", ";
						s << p->to_string( indent );
					}
					s << " ]";
					break;
				case NODE_TYPE::STRING:
					s << "\"" << encode(str) << "\"";
					break;
				case NODE_TYPE::BOOL:
					s << ( boolean ? "true" : "false" );
					break;
				case NODE_TYPE::UINT:
					*reinterpret_cast< std::ostringstream* >( &s ) << uint;
					break;
				case NODE_TYPE::INT:
					*reinterpret_cast< std::ostringstream* >( &s ) << integer;
					break;
				case NODE_TYPE::FLOAT:
					*reinterpret_cast< std::ostringstream* >( &s ) << floating;
					break;
				case NODE_TYPE::EMPTY:
					s << "null";
					break;
				default:
					throw std::invalid_argument( "invald type" );
				};
			}
			catch( std::exception& e )
			{
				( e );
			}
			return s.str();
		};

		// parses a string into an object
		bool parse( std::istream&& ss)
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
					new( &object ) Object();

					while( 1 )
					{
						ss >> c;
						if( '"' == c )
						{
							// rewind to catch the quote
							ss.seekg( -1, std::ios::cur );
							std::string name;
							ss >> quoted( name, "\"" ) >> c;
							if( ':' == c )
							{
								Value v;
								if( v.parse( ( std::istream&& )ss ) )
									object.emplace( decode( name ), std::move( v ) );
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
					new( &object ) List();

					while( 1 )
					{
						Value v;
						if( v.parse( ( std::istream&& )ss ) )
							lst.emplace_back( v );
						else
							return false;
						// try read a comma
						ss >> c;
						if( ',' != c )
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
					ss >> quoted( value );

					if( !value.empty() )
					{
						if( !hasQuote )
						{
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
									double d = std::stod( *reinterpret_cast< std::string* >( &value ) );
									uint64_t u = std::stoull( *reinterpret_cast< std::string* >( &value ) );
									uint64_t i = std::stoll( *reinterpret_cast< std::string* >( &value ) );
									if( d >= 0 && d == u )
									{
										type = NODE_TYPE::UINT;
										uint = u;
									}
									else if( d == i )
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
							new( &str ) std::u8string( decode( value ) );
							type = NODE_TYPE::STRING;
							return true;
						}
					}
				}
			}
			return false;
		}
		bool parse( std::string const& s )
		{
			return parse( std::istringstream( s ) );
		}

	};

	static std::string encode( const std::u8string& in )
	{
		std::string s; s.reserve( in.size() * 10 / 9 );
		for( auto it = in.begin(); it != in.end(); it++ )
		{
			auto u = *it;
			if( u > 127 ) // unicode non-latin characters
			{
				s.push_back( '\\' );
				s.push_back( 'u' );
				unsigned int v; // 3 bytes max for JSON, to encode up tp \UFFFF
				// first most significant two bits must be 11, so we ignore them
				// we need to check MS bit 3, if zero we got a 2 byte encoding
				if( 0 == ( u & 0b00100000 ) )
				{
					// the last 5 bits go to bits 12 to 7
					v = unsigned int( u & 0b00011111 ) << 6;
					if( ++it != in.end() )
						u = *it;
					// the next char must start with 10, so we take last 6 bits
					v |= unsigned int( u & 0b00111111 );
				}
				// now we must check MS bit 4, if zero we got a 3 byte encoding
				else if( 0 == ( u & 0b00010000 ) )
				{
					// the last 4 bits go to bits 17 to 13
					v = unsigned int( u & 0b00001111 ) << 12;
					if( ++it != in.end() )
					{
						u = *it;
						// the next char must start with 10, so we take last 6 bits and put them to bits 12 to 7
						v |= unsigned int( u & 0b00111111 ) << 6;
						if( ++it != in.end() )
						{
							u = *it;
							// the next char must start with 10, so we take last 6 bits and put them to bits 6 to 1
							v |= unsigned int( u & 0b00111111 );
						}
						else // we file an error
							v = 0xFFFF;
					}
					else // we file an error
						v = 0xFFFF;
				}
				// now we must check MS bit 5, if zero we got a 4 byte encoding
				else if( 0 == ( u & 0b00001000 ) )
				{
					// the last 3 bits go to bits 22 to 19
					v = unsigned int( u & 0b00000111 ) << 18;
					if( ++it != in.end() )
					{
						u = *it;
						// the next char must start with 10, so we take last 6 bits and put them to bits 18 to 13
						v |= unsigned int( u & 0b00111111 ) << 12;
						if( ++it != in.end() )
						{
							u = *it;
							// the next char must start with 10, so we take last 6 bits and put them to bits 12 to 7
							v |= unsigned int( u & 0b00111111 ) << 6;
							if( ++it != in.end() )
							{
								u = *it;
								// the next char must start with 10, so we take last 6 bits and put them to bits 6 to 1
								v |= unsigned int( u & 0b00111111 );
							}
							else // we file an error
								v = 0xFFFF;
						}
						else // we file an error
							v = 0xFFFF;
					}
					else // we file an error
						v = 0xFFFF;
				}
				// now we must check MS bit 6, if zero we got a 5 byte encoding
				else if( 0 == ( u & 0b00000100 ) )
				{
					// we read next 4 bytes and write FFFF, as we cannot encode 6 byte
					for( int i = 0; i != 4; i++ ) if( ++it == in.end() ) break;
					v = 0xFFFF;
				}
				// must be a 6 byte encoding...
				else
				{
					// we read next 5 bytes and write FFFF, as we cannot encode 6 byte
					for( int i = 0; i != 5; i++ ) if( ++it == in.end() ) break;
					v = 0xFFFF;
				}

				std::ostringstream ss;
				ss << std::hex << std::setfill( '0' ) << std::setw( 4 ) << v;
				s.append( ss.str() );
			}
			else
			{
				// needs quotation
				if( '\\' == u || '/' == u || '"' == u )
				{
					s.push_back( '\\' );
					s.push_back( u );
				}
				else if( u < 32 ) // unicode control codes
				{
					// special chars
					if( '\t' == u )
					{
						s.push_back( '\\' );
						s.push_back( 't' );
					}
					else if( '\n' == u )
					{
						s.push_back( '\\' );
						s.push_back( 'n' );
					}
					else if( '\r' == u )
					{
						s.push_back( '\\' );
						s.push_back( 'r' );
					}
					else if( '\b' == u )
					{
						s.push_back( '\\' );
						s.push_back( 'b' );
					}
					else if( '\f' == u )
					{
						s.push_back( '\\' );
						s.push_back( 'f' );
					}
					else
					{
						s.push_back( '\\' );
						s.push_back( 'u' );
						std::ostringstream ss;
						ss << std::hex << std::setfill( '0' ) << std::setw( 4 ) << char( u );
						s.append( ss.str() );
					}
				}
				else
					s.push_back( u );
			}

			if( it == in.end() )
				break;
		}
		return s;
	}

	static std::u8string decode( const std::string& in ) 
	{
		std::u8string s; s.reserve( in.size() * 10 / 9 );
		for( auto it = in.begin(); it != in.end(); it++ )
		{
			auto c = *it;
			if( c == '\\' )
			{
				if( ++it != in.end() )
				{
					c = *it;
					switch( c )
					{
					case 't':
					case 'T':
						s.push_back( '\t' );
						break;
					case 'n':
					case 'N':
						s.push_back( '\n' );
						break;
					case 'r':
					case 'R':
						s.push_back( '\r' );
						break;
					case 'b':
					case 'B':
						s.push_back( '\b' );
						break;
					case 'f':
					case 'F':
						s.push_back( '\f' );
						break;
					case 'u':
					case 'U':
					{
						// read 4 digits
						char h[5]{};
						for( int i = 0; i != 4; i++ )
						{
							if( ++it != in.end() )
							{
								h[i] = ( char8_t )std::u8string::value_type( *it );
							}
							else
								break;
						}

						unsigned int v = strtoul( h, nullptr, 16 );
						if( v <= 0x7F )
						{
							s.push_back( std::u8string::value_type( v ) );
						}
						else if( v <= 0x7FF ) // 2 bytes
						{
							s.push_back( 0b11000000 | std::u8string::value_type( ( v >> 6 ) & 0b00011111 ) );
							s.push_back( 0b10000000 | std::u8string::value_type( v & 0b00111111 ) );
						}
						else // 3 bytes
						{
							s.push_back( 0b11100000 | std::u8string::value_type( ( v >> 12 ) & 0b00001111 ) );
							s.push_back( 0b10000000 | std::u8string::value_type( ( v >> 6 ) & 0b00111111 ) );
							s.push_back( 0b10000000 | std::u8string::value_type( v & 0b00111111 ) );
						}
					}
					break;
					default:
						s.push_back( std::u8string::value_type( c ) );
						break;
					}
				}
			}
			else
			{
				s.push_back( std::u8string::value_type( c ) );
			}
		}
		return s;
	}

protected:
	Value root;
	int state;

public:
	typedef Value::Object Object;
	typedef Value::List List;

	JSON() : state( 1 ), root( Value( Object( {} ) ) ) {}
	JSON( JSON const& ) = default;
	JSON( JSON&& ) = default;

	JSON( std::istream&& s ) : JSON() { state = root.parse( (std::istream&&)s ); }
	JSON( std::string const& s ) : JSON( std::istringstream( s ) ) {}
	JSON( char const* s ) : JSON( std::istringstream( s ) ) {}
	JSON( Object const& obj ) : root( Value( obj ) ), state( 1 ) {}
	JSON( Object&& obj ) noexcept : root( std::move( Value( std::move( obj ) ) ) ), state( 1 ) {}

	~JSON() {}
	std::string to_string() const { return root.to_string(); }
	bool good() const { return 0 != state; }
	bool bad() const { return 0 == state; }
	bool empty() const { return root.type == NODE_TYPE::EMPTY; }
	// not needed, as root must be an object
	//operator Value const&() const { return root; };
	//operator Value&() { return root; };
	//Value const& operator[]( std::string tag ) const { return root[tag];}
	Value& operator[]( std::u8string const& tag ) { return root[tag]; }
	Value const& operator[]( std::u8string const& tag ) const { return root[tag]; }
	Value& operator[]( char8_t const* tag ) { return root[tag]; }
	Value const& operator[]( char8_t const* tag ) const { return root[tag]; }
	Object::iterator begin() { return root.begin<Object>(); }
	Object::const_iterator begin() const { return root.begin<Object>(); }
	Object::iterator end() { return root.end<Object>(); }
	Object::const_iterator end() const { return root.end<Object>(); }
	Object::iterator find( std::u8string const& tag ) { return root.find( tag ); }
	Object::const_iterator find( std::u8string const& tag ) const { return root.find( tag ); }
	size_t size() const { return root.size<Object>(); }
};

//class JSONRPC : public JSON
//{
//public:
//	JSON();
//	JSON(std::string s);
//	JSON(std::istream s);
//	virtual ~JSON();
//
//protected:
//	int id;
//	std::string method;
//};
