#include "JSONWrapper.h"
#include <sstream>


JSONWrapper::JSONWrapper()
	: m_id(0)
{
}

JSONWrapper::JSONWrapper( std::string s )
{
	// check version

// parse to map

// 
}

JSONWrapper::JSONWrapper( std::istream s )
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

