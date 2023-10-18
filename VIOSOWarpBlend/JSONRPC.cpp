#include "Platform.h"
#include "JSONRPC.h"
#include "logging.h"

using namespace std;

unique_ptr<JSONRPCReactor> JSONRPCReactor::_;

TCPProto::ParserAdder JSONRPC::_adder( parse );
TCPProto::TYPE JSONRPC::myType( "JSON" );

TCPProto::ptr_t JSONRPC::parse( TCPConnection& conn )
{
	
	if( conn.cmpFront( "{" ) || conn.cmpFront( "[" ) )
	{
		return make_unique<JSONRPC>( conn );
	}

	return ptr_t();
}

bool JSONRPC::send( TCPConnection& conn )
{
	return false;
}

