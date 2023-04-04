#include "Platform.h"
#include "HTTP.h"
#include <limits>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
using namespace std;

int HttpBase::parseURL( std::string const& request, std::string& site, ParamMap& getData )
{
	if( request.empty() )
		return SOCKET_ERROR;

	std::string::size_type pos = request.find( '?' );
	if( std::string::npos != pos )
	{
		std::string::size_type pos1 = pos + 1;
		while( 1 )
		{
			std::string::size_type pos2 = request.find( '&', pos1 );
			std::string::size_type pos3 = request.find( '=', pos1 );
			if( std::string::npos == pos3 )
				break;
			getData[std::string( request, pos1, pos3 - pos1 )] = std::string( request, pos3, std::string::npos == pos2 ? std::string::npos : pos2 - pos3 );
			if( std::string::npos == pos2 )
				break;
			pos1 = pos2 + 1;
		}
		site = request.substr( 0, pos - 1 );
		return 1 + ( int )getData.size();
	}
	else
		site = request;
	return 1;
}

int HttpBase::parseHttpHeader( char const* szIn, HTTYPE& type, std::string& request, ParamMap& headers )
{
	if( NULL == szIn )
		return -1;
	char const* p = szIn;
	char const* pE = strstr( p, "\015\012" );
	if( NULL == pE )
		return SOCKET_ERROR;
	request = std::string( p, pE - p );

	std::string::size_type po1 = request.find( ' ', 5 ); // this is the space before protocol, this is mandatory but does not change change processing
	if( std::string::npos == po1 )
		return SOCKET_ERROR;

	if( 0 == request.compare( 0, 4, "GET " ) )
	{
		type = HTTYPE_GET;
		request = request.substr( 4, po1 - 4 );
	}
	else if( 0 == request.compare( 0, 5, "POST " ) )
	{
		type = HTTYPE_POST;
		request = request.substr( 5, po1 - 5 );
	}
	else
		return SOCKET_ERROR;

	// read header lines
	p = pE + 2;
	parseHeader( p, headers );
	return 1 + ( int )headers.size();
}

int HttpBase::parseHeader( char const* p, ParamMap& headers )
{
	char const* pS = p;
	char const* pE = NULL;
	while( *p && NULL != ( pE = strstr( p, "\015\012" ) ) )
	{
		std::string s( p, pE - p );
		if( s.empty() )
			return ( int )( p - pS ) + 2;
		std::string::size_type pos = s.find( ':' );
		if( std::string::npos != pos )
		{
			headers[std::string( p, pos )] = std::string( s, pos + 2, std::string::npos );
		}
		else
			headers[std::string( p )] = "";
		p = pE + 2;
	}
	return SOCKET_ERROR;
}

int HttpBase::parseHeaderLine( char const* p, ParamMap& params )
{
	char const* pE = NULL;
	while( p )
	{
		pE = strstr( p, "; " );

		std::string s;
		if( pE )
		{
			s = std::string( p, pE - p );
			pE += 2;
		}
		else
			s = p;

		std::string::size_type pos = s.find( '=' );
		if( std::string::npos != pos )
		{
			if( '"' == s[pos + 1] )
			{
				params[std::string( p, pos )] = std::string( s, pos + 2, -1 );
				params[std::string( p, pos )].pop_back();
			}
			else
				params[std::string( p, pos )] = std::string( s, pos + 1, -1 );
		}
		else
		{
			params[s] = "";
		}
		p = pE;
	}

	return ( int )params.size();
}

int HttpBase::parseURLencBody( std::string const& body, PostParamMap& postData )
{
	if( body.empty() )
		return SOCKET_ERROR;

	std::string::size_type pos1 = 0;
	while( 1 )
	{
		std::string::size_type pos2 = body.find( '&', pos1 );
		std::string::size_type pos3 = body.find( '=', pos1 );
		if( std::string::npos == pos3 )
			break;
		PostValue v = { ENCTYPE_URL, std::string( body, pos3 + 1, std::string::npos == pos2 ? std::string::npos : pos2 - pos3 - 1 ), "" };
		postData[std::string( body, pos1, pos3 - pos1 )] = v;
		if( std::string::npos == pos2 )
			break;
		pos1 = pos2 + 1;
	}
	return ( int )postData.size();
}

int HttpBase::parseMultipartBody( std::string const& body, std::string const& bound, PostParamMap& postData )
{
	if( bound.length() * 2 + 10 > body.size() )
		return SOCKET_ERROR;
	std::string::size_type lBound = bound.length();
	std::string::size_type found = std::string::npos;

	// must start with bound
	if( 0 == body.compare( 0, lBound, bound ) )
	{
		std::string::size_type offs = lBound + 2; // step over first boundary line feed
		std::string b2( "\015\012" ); b2.append( bound ); lBound += 2;
		while( std::string::npos != ( found = body.find( b2, offs ) ) )
		{
			// next boundary found; inbetween there is content
			std::string blob = body.substr( offs, found - offs );
			offs = found + lBound + 2;

			// parse blob
			ParamMap headers;
			int datastart = parseHeader( blob.c_str(), headers );
			if( SOCKET_ERROR != datastart )
			{
				//std::string cntt = "ContentDissition: form-data; ";
				ParamMap::iterator itCD = headers.find( "Content-Disposition" );
				ParamMap::iterator itCT = headers.find( "Content-Type" );
				if( headers.end() != itCD && headers.end() != itCT )
				{
					ParamMap params;
					parseHeaderLine( itCD->second.c_str(), params );
					ParamMap::iterator itName = params.find( "name" );
					ParamMap::iterator itFile = params.find( "filename" );
					if( params.end() != itName )
					{
						PostValue& v = postData[itName->second];
						v.encType = ENCTYPE_OTHER;
						v.value = blob.substr( datastart, std::string::npos );
						if( params.end() != itFile )
						{
							v.file.swap( itFile->second );
						}
					}
				}
			}

			if( '-' == body[offs] )
				return int( offs + 4 ); // finished NOTE: conversion to INT is sane, as the content size cannot exceed 2GB
			else
				return SOCKET_ERROR;
		}
	}
	return SOCKET_ERROR; // did not find final boundary, this is an error
}

int HttpBase::resolveURL( std::string const& url, bool& isSecure, std::string& host, unsigned short& port, std::string& path, ParamMap& getParams, std::string& fragment )
{
	// to resolve, the url must be at least 8 characters wide, http://a
	// get protocol
	if( 8 < url.size() && !strncmp( "http", url.c_str(), 4 ) && ( !strncmp( "://", &url.c_str()[4], 3 ) || !strncmp( "s://", &url.c_str()[4], 4 ) ) )
	{
		size_t posHost = 7;
		if( 's' == url.c_str()[4] )
		{
			isSecure = true;
			posHost++;
		}
		else
			isSecure = false;

		auto posPath = url.find( '/', 8 );
		auto posParams = url.find( '?', 8 );
		auto posFragment = url.find( '#', 8 );
		if( posFragment <= posPath ) // there is no path before start of fragment
		{
			posPath = string::npos; // the '/' was found after the fragment qualifier, so its part of the fagment rather than indicating the beginning of the resource path, thus ist ends rather with the params qualifier
		}
		if( posFragment <= posParams )  // there is no params before fragment
		{
			posParams = string::npos;
		}
		if( posParams <= posPath )  // there is no path before params (resp. before fragment)
		{
			posPath = string::npos;
		}
		if( string::npos == posPath ) // no path
		{
			if( posParams < posFragment ) // read until params
			{
				host = url.substr( posHost, posParams - posHost );
			}
			else // read until fragment (or to end, if it is npos)
			{
				host = url.substr( posHost, posFragment - posHost );
			}
		}
		else // read until path
		{
			host = url.substr( posHost, posPath - posHost );
		}
		// the host might contain a port
		auto posPort = host.find( ':', 1 );
		port = 0;
		if( string::npos != posPort )
		{
			port = ( unsigned short )stoi( host.substr( posPort + 1 ) );
			host.erase( posPort, string::npos );
		}
		if( 0 == port )
		{
			if( isSecure )
				port = 443;
			else
				port = 80;
		}

		// get the path
		if( string::npos != posPath )
		{
			if( posParams < posFragment ) // there are parameters
				path = URLencode( url.substr( posPath, posParams - posPath ) ); // we need the /, so no +1
			else
				path = URLencode( url.substr( posPath, posFragment - posPath ) );
		}
		else
			path = "/";

		// get the getParams
		getParams.clear();
		for( auto pos = posParams; pos < posFragment; )
		{
			auto pos2 = url.find( '&', pos + 1 );
			if( pos2 >= posFragment ) // the & was in fragment
				pos2 = posFragment;
			auto pos3 = url.find( '=', pos + 1 );
			string value;
			if( pos3 > pos2 ) // no value
				pos3 = pos2;
			else
				value = url.substr( pos3 + 1, pos2 - pos3 - 1 );
			string key = url.substr( pos + 1, pos3 - pos - 1 );
			getParams.emplace( URLencode( key ), URLencode( value ) );
			pos = pos2;
		}

		// get the fragment
		if( string::npos != posFragment )
		{
			fragment = URLencode( url.substr( posFragment + 1 ) );
		}
		return 1;
	}
	return 0;
}

std::string HttpBase::URLencode( std::string const& unencoded )
{
	// note: we do not encode '/'
	std::string s; s.reserve( unencoded.size() );
	for( auto c : unencoded )
	{
		if(
			( '0' <= c && c <= '9' ) ||
			( 'a' <= c && c <= 'z' ) ||
			( 'A' <= c && c <= 'Z' ) ||
			( '.' == c || '~' == c || '-' == c || '_' == c || '/' == c )
			)
		{
			s.push_back( c );
		}
		else if( ' ' == c )
		{
			s.push_back( '+' );
		}
		else
		{
			s.push_back( '%' );
			unsigned char x;
			x = unsigned char( c ) >> 4;
			if( 9 < x )
				x += 'A' - 9;
			else
				x += '0';
			s.push_back( x );
			x = unsigned char( c ) & 0x0F;
			if( 9 < x )
				x += 'A' - 9;
			else
				x += '0';
			s.push_back( x );
		}
	}
	return s;
}

std::string HttpBase::URLdecode( std::string const& encoded )
{
	std::string s; s.reserve( encoded.size() );
	for( auto c = encoded.begin(); c != encoded.end(); c++ )
	{
		if( '+' == *c )
		{
			s.push_back( '+' );
		}
		else if( '%' == *c )
		{
			unsigned char xHi = *( ++c );
			if( 'A' <= xHi )
				xHi -= 'A' + 9;
			else
				xHi -= '0';
			unsigned char x = *( ++c );
			if( 'A' <= x )
				x -= 'A' + 9;
			else
				x -= '0';
			x += xHi << 4;
			s.push_back( x );
		}
		else
			s.push_back( *c );
	}
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////
// HttpRequest

TCPProto::ParserAdder HttpRequest::_adder( parse );
TCPProto::TYPE HttpRequest::myType( "HTTPREQU" );

TCPProto::ptr_t HttpRequest::parse( TCPConnection& conn )
{
	if( conn.cmpFront( "GET " ) || conn.cmpFront( "POST" ) )
	{
		return make_unique<HttpRequest>( conn );
	}

	return ptr_t();
}


HttpRequest::HttpRequest( TCPConnection& conn )
	: TCPProto(myType), httype( HTTYPE_UNDEF ), enctype( ENCTYPE_UNDEF )
{
	char* pB = NULL;
	int nB = 0;
	switch( state )
	{
	case STATE_UNDEF:
	{
		state = STATE_INIT;
	}
	[[fallthrough]];
	/* fall through */
	case STATE_INIT:
	{
		nB = conn.getNumRead();
		pB = new char[nB];
		int n = conn.readUntil( pB, nB, "\015\012\015\012" );
		if( 0 < n )
		{
			if( 0 < parseHttpHeader( pB, httype, content, headers ) )
				state = STATE_HEADER;
			else
			{
				state = STATE_ERROR;
				break;
			}
		}
		else if( 0 > n )
		{
			state = STATE_ERROR;
			break;
		}
		else
			break;
	}
	[[fallthrough]];
	/* fall through */
	case STATE_HEADER:
	{
		parseURL( content, content, getData );
		// read content length, this is a must on post requests
		if( HTTYPE_POST == httype )
		{
			auto it = headers.find( "Content-Type" );
			if( it != headers.end() )
			{
				if( 0 == it->second.compare( "application/x-www-form-urlencoded" ) )
					enctype = ENCTYPE_URL;
				else if( 0 == it->second.compare( 0, 19, "multipart/form-data" ) )
				{
					enctype = ENCTYPE_MULTI;
					bound = std::string( "--" );
					bound.append( it->second.substr( 30 ) );
				}
				else // other
				{
					enctype = ENCTYPE_OTHER;
					bound = it->second.substr( 0, it->second.find( ';' ) );
				}
				headers.erase( it );
			}
			else
				enctype = ENCTYPE_URL;
			it = headers.find( "Content-Length" );
			if( it != headers.end() )
			{
				contentLength = atoi( it->second.c_str() );
				if( 0 < contentLength )
				{
					state = STATE_CONTENTLENGTH;
				}
				else
				{
					state = STATE_ERROR;
					break;
				}
				headers.erase( it ); // need to remove from headers after parsed
			}
			else
			{
				state = STATE_ERROR;
				break;
			}
		}
		else
		{
			state = STATE_PARSED;
			break;
		}
	}
	[[fallthrough]];
	/* fall through */
	case STATE_CONTENTLENGTH:
	{
		if( contentLength > conn.getNumRead() )
			break;

		body.resize( size_t( contentLength ) + 1 );

		if( contentLength == conn.read( &body[0], contentLength + 1, contentLength ) )
			state = STATE_BODY;
		else
			state = STATE_ERROR;
	}
	[[fallthrough]];
	/* fall through */
	case STATE_BODY:
	{
		int i;
		if( ENCTYPE_URL == enctype )
			i = parseURLencBody( body, postData );
		else
			i = parseMultipartBody( body, bound, postData );
		if( 0 < i )
			state = STATE_PARSED;
		else if( 0 == i )
			break;
		else
		{
			state = STATE_ERROR;
			break;
		}
	}
	[[fallthrough]];
	/* fall through */
	case STATE_PARSED:
		break;
	}
	if( pB )
		delete[] pB;
}

HttpRequest::HttpRequest( HTTYPE type, std::string url, PostParamMap const& postParams, ParamMap const& headers )
	: TCPProto( myType, url )
	, httype( type )
	, enctype( ENCTYPE_URL )
	, bound( "-----------------------------VIOSOAPI" )
	, contentLength( 0 )
	, postData( postParams )
	, HttpRequest::headers( headers )
{
}

bool HttpRequest::send( TCPConnection& conn )
{
	std::stringstream s( std::ios_base::binary );
	if( postData.empty() ) // GET header
		httype = HTTYPE_POST;
	else if( HTTYPE_UNDEF == httype )
		httype = HTTYPE_GET;

	if( HTTYPE_POST == httype )
	{
		s << "GET ";
	}
	else if( HTTYPE_HEAD == httype ) // HEAD header
	{
		s << "HEAD ";
	}
	else if( HTTYPE_POST == httype ) // POST header
	{
		s << "POST ";
	}
	else
		return false;

	if( content.empty() )
		s << "/";
	else
		s << content;
	if( !getData.empty() )
	{
		s << "?";
		ParamMap::const_iterator param = getData.begin();
		s << param->first;
		if( !param->second.empty() )
			s << ":" << param->second;
		for( ; param != getData.end(); param++ )
		{
			s << "&" << param->first;
			if( !param->second.empty() )
				s << ":" << param->second;
		}
	}
	s << " HTTP/1.1\015\012";
	for( ParamMap::const_iterator header = headers.begin(); header != headers.end(); header++ )
	{
		s << header->first;
		if( !header->second.empty() )
			s << ": " << header->second;
		s << "\015\012";
	}
	// calculate content size, if POST

	if( !postData.empty() )
	{
		s << "Content-Type: multipart/form-data; boundary=" << bound << "\015\012";
		std::stringstream ss;
		for( PostParamMap::const_iterator it = postData.begin(); it != postData.end(); it++ )
		{
			ss << "--" << bound << "\015\012";
			ss << "Content-Disposition: form-data; name=\"" << it->first << "\"";
			if( !it->second.file.empty() )
			{
				ss << "; filename = \"" << it->second.value << "\"";
			}
			ss << "\015\012";
			if( it->second.file.empty() )
			{
				ss << "Content-Type: text/plain" << "\015\012";
				ss << "\015\012";
				ss << it->second.value << "\015\012";
			}
			else
			{
				switch( it->second.encType )
				{
				case ENCTYPE_UNDEF:
					ss << "Content-Type: application/octet-stream" << "\015\012";
					break;
				case ENCTYPE_URL:
					ss << "Content-Type: application/x-url-encoded" << "\015\012";
					break;
				case ENCTYPE_MULTI: // actually an error
				case ENCTYPE_OTHER:
					ss << "Content-Type: application/octet-stream" << "\015\012";
					break;
				}
				ss << "\015\012";
				ss << it->second.file;
				ss << "\015\012";
			}
		}
		ss << "--" << bound << "--" << "\015\012";
		s << "Content-Length: " << ss.str().size() << "\015\012";
		s << "\015\012";
		s << ss.str();
	}

	return 0 < conn.write( s.str().c_str(), ( int )s.str().size() );
}

/////////////////////////////////////////////////////////////////////////////

HttpResponse::HttpResponse( TCPConnection& conn )
	: TCPProto( TYPE( "HTTPRESP" ) )
	, code( 0 )
	, contentIsPath(false)
{
}

HttpResponse::HttpResponse( int code_, std::string const& content, bool contentIsPath_, std::string const& ctype, ParamMap const& headers_ )
: TCPProto( TYPE( "HTTPRESP"), content )
, code( code_ )
, contentIsPath( contentIsPath_ )
, headers( headers_ )
{
	if( ctype.empty() )
	{
		if( contentIsPath )
		{
			static const std::map< string, string, strcmpFn > assoc({
				{ ".bmp",  "image/bmp" },
				{ ".jpg",  "image/jpeg" },
				{ ".jpeg", "image/jpeg" },
				{ ".png",  "image/png" },
				{ ".gif",  "image/gif" },
				{ ".txt",  "text/plain" },
				{ ".js",   "text/javascript" },
				{ ".json", "text/json" },
				{ ".css",  "text/css" },
				{ ".xml",  "text/xml" },
				{ ".html", "text/html" },
				{ ".htm",  "text/html" },
				{ ".ics",  "text/calendar" },
				{ ".csv",  "text/csv" },
				{ ".mp3",  "audio/mp3" },
				{ ".mpg",  "video/mpeg" },
				{ ".mpeg", "video/mpeg" },
				{ ".mp4",  "video/mp4" },
				{ ".rtf",  "application/rtf" },
				{ ".jar",  "application/java-archive" },
				{ ".pdf",  "application/pdf" },
				{ ".ttf",  "font/ttf" },
				{ ".woff", "font/woff" } });
			
			auto extIt = assoc.find( filesystem::path( content ).extension().string() );
			if( extIt != assoc.end() )
				headers["Content-Type"] = extIt->second;
			else
				headers["Content-Type"] = "application/octet-stream";
		}
		else
			headers["Content-Type"] = "text/html";
	}
}

bool HttpResponse::send( TCPConnection& conn )
{
	std::stringstream s( std::ios_base::binary );
	unique_ptr<istream> cnt;
	int sz = 0;
	if( contentIsPath )
	{
		cnt = make_unique<ifstream>( content, std::ios_base::in | std::ios_base::binary );
	}
	else
		cnt = make_unique<istringstream>( content );

	if( cnt->fail() )
		s << "404 Not Found" << "\015\012";
	else
	{
		// update size header
		{
			cnt->seekg( 0, ios_base::end );
			sz = (int)cnt->tellg();
			cnt->seekg( 0 );
			headers["Content-Length"] = to_string( sz );
		}
		s << "200 OK" << "\015\012";
		for( auto const& header : headers )
		{
			s << header.first << ": " << header.second << "\015\012";
		}
	}
	s << "\015\012";
	if( !cnt->fail() )
	{
		int sps = 0;
		size_t hdsz = s.tellp();
		if( 0 == conn.getSendBuffSize(sps) && 
			size_t( sps ) > hdsz &&
			size_t( sps ) - hdsz > sz )
		{
			s << cnt;
			conn.write( s.str().data(), (int)s.str().length() );
		}
		else
		{
			conn.write( s.str().data(), (int)s.str().length() );
			string s( "", sps );
			while( !cnt->eof() )
			{
				conn.write(s.data(), (int)cnt->readsome(s.data(), sps));
			}
		}
	}


	return 0;
}
