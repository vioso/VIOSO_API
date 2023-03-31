#include "Platform.h"
#include "JSON.h"
#include "logging.h"

using namespace std;

TCPProto::ParserAdder Json::_adder( parse );
TCPProto::TYPE Json::myType( "JSON" );

TCPProto::ptr_t Json::parse( TCPConnection& conn )
{
	
	if( conn.cmpFront( "{" ) || conn.cmpFront( "[" ) )
	{
		return make_unique<Json>( conn );
	}

	return ptr_t();
}

bool Json::send( TCPConnection& conn )
{
	return false;
}

