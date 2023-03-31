#pragma once
#include "TCP.h"

class Json : public TCPProto
{
protected:
	static ParserAdder _adder;
public:
	static ptr_t parse( TCPConnection& conn );
	static TYPE myType;
	Json( TCPConnection& conn ) : TCPProto( myType ) { state = STATE_ERROR; }
	Json( std::string const& cont ) : TCPProto( myType, cont ) {}
	virtual bool send( TCPConnection& conn );
};
