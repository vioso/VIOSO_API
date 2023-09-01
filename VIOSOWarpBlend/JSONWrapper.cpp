#include "JSONWrapper.h"
#include <sstream>


JSONWrapper::JSONWrapper()
	: id(0)
	, prettyPrint(false)
{
}

JSONWrapper::JSONWrapper( std::string s )
	: JSONWrapper()
{
	// check version

// parse to map

// 
}

JSONWrapper::JSONWrapper( std::istream s )
	: JSONWrapper()
{
	// check version

// parse to map

// 
}


JSONWrapper::~JSONWrapper()
{
}

std::string JSONWrapper::to_string() const
{
	return params.to_string();
}

