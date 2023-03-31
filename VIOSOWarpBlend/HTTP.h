#pragma once
#include "TCP.h"

class HttpBase
{
public:
	typedef enum HTTYPE {
		HTTYPE_UNDEF,
		HTTYPE_GET,
		HTTYPE_HEAD,
		HTTYPE_POST,
		HTTYPE_DELETE,
		HTTYPE_PUT
	} HTTYPE;

	typedef enum ENCTYPE {
		ENCTYPE_UNDEF,
		ENCTYPE_URL,
		ENCTYPE_MULTI,
		ENCTYPE_OTHER
	} ENCTYPE;

	typedef struct PostValue {
		ENCTYPE encType{ ENCTYPE_UNDEF };
		std::string value;
		std::string file;
	} PostValue;
	struct strcmpFn { bool operator()( std::string const& x, std::string const& y ) const { return 0 < x.compare( y ); }; };

	typedef std::map< std::string, std::string, strcmpFn > ParamMap;
	typedef std::map< std::string, PostValue, strcmpFn > PostParamMap;

	static int parseURL( std::string const& url, std::string& site, ParamMap& getParams );
	static int parseHttpHeader( char const* szIn, HTTYPE& type, std::string& request, ParamMap& heads );
	static int parseHeader( char const* szIn, ParamMap& heads );
	static int parseHeaderLine( char const* p, ParamMap& params );
	static int parseURLencBody( std::string const& body, PostParamMap& postData );
	static int parseMultipartBody( std::string const& body, std::string const& bound, PostParamMap& postData );
	static int resolveURL( std::string const& url, bool& isSecure, std::string& host, unsigned short& port, std::string& path, ParamMap& getParams, std::string& fragment );
	static std::string URLencode( std::string const& unencoded );
	static std::string URLdecode( std::string const& encoded );
};

class HttpRequest : public TCPProto, HttpBase
{
protected:
	static ParserAdder _adder;
public:
	typedef enum STATE {
		STATE_HEADER = STATE_INIT + 1,
		STATE_CONTENTLENGTH,
		STATE_BODY,
	} STATE;

	static ptr_t parse( TCPConnection& conn );
	static TYPE myType;

	HTTYPE			httype;
	ENCTYPE			enctype;
	std::string		bound;
	int				contentLength;
	ParamMap		getData;
	ParamMap		headers;
	std::string		body;
	PostParamMap	postData;


	HttpRequest() : TCPProto(myType), httype( HTTYPE_UNDEF ), enctype( ENCTYPE_UNDEF ), contentLength( 0 ) {};
	HttpRequest( HttpRequest const& other ) noexcept
		: TCPProto( other )
		, httype( other.httype )
		, enctype( other.enctype )
		, getData( other.getData )
		, headers( other.headers )
		, body( other.body )
		, postData( other.postData )
		, bound( other.bound )
		, contentLength( other.contentLength )
	{};
	HttpRequest( HttpRequest&& other ) noexcept
		: TCPProto( std::move( other ) )
		, httype( other.httype )
		, enctype( other.enctype )
		, getData( std::move( other.getData ) )
		, headers( std::move( other.headers ) )
		, body( std::move( other.body ) )
		, postData( std::move( other.postData ) )
		, bound( std::move( other.bound ) )
		, contentLength( other.contentLength )
	{};

	//HttpRequest& operator=( HttpRequest const& other ) noexcept
	//{
	//	Request::operator=( other );
	//	httype = other.httype;
	//	enctype = other.enctype;
	//	getData = other.getData;
	//	headers = other.headers;
	//	return *this;
	//}

	//HttpRequest& operator=( HttpRequest&& other ) noexcept
	//{
	//	Request::operator=( std::move( other ) );
	//	httype = other.httype;
	//	enctype = other.enctype;
	//	getData.swap( other.getData );
	//	headers.swap( other.headers );
	//	return *this;
	//}

	HttpRequest( TCPConnection& conn );

	HttpRequest( HTTYPE httype, std::string request, PostParamMap const& postParams = {}, ParamMap const& headers = {} );

	virtual bool send( TCPConnection& conn );
};

/////////////////////////////////////////////////////////////////////////////

class HttpResponse : public TCPProto, HttpBase {
protected:
	int code;
	bool contentIsPath;
	ParamMap headers;
public:
	HttpResponse( TCPConnection& conn );
	// setting ctype empty tries to guess it from content
	HttpResponse( int code, std::string const& content, bool contentIsPath = false, std::string const& ctype = "", ParamMap const& headers = {});
	
	HttpResponse( HttpResponse const& other ) noexcept
		: TCPProto( other )
		, code( other.code )
		, contentIsPath( other.contentIsPath )
		, headers( other.headers )
	{};
	HttpResponse( HttpResponse&& other ) noexcept
		: TCPProto( std::move( other ) )
		, code( other.code )
		, contentIsPath( other.contentIsPath )
		, headers( std::move( other.headers ) )
	{};

	virtual bool send( TCPConnection& conn );
};
