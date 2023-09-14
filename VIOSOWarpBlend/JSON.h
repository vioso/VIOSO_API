#pragma once
#include "TCP.h"

class JSON : public TCPProto
{
protected:
	static ParserAdder _adder;
public:
	static ptr_t parse( TCPConnection& conn );
	static TYPE myType;
	JSON( TCPConnection& conn ) : TCPProto( myType ) { state = STATE_ERROR; }
	JSON( std::string const& cont ) : TCPProto( myType, cont ) {}
	virtual bool send( TCPConnection& conn );
};
