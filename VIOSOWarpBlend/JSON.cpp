#include "Platform.h"
#include "JSON.h"
#include "logging.h"

using namespace std;

TCPProto::ParserAdder JSON::_adder( parse );
TCPProto::TYPE JSON::myType( "JSON" );

TCPProto::ptr_t JSON::parse( TCPConnection& conn )
{
	
	if( conn.cmpFront( "{" ) || conn.cmpFront( "[" ) )
	{
		return make_unique<JSON>( conn );
	}

	return ptr_t();
}

bool JSON::send( TCPConnection& conn )
{
	return false;
}

