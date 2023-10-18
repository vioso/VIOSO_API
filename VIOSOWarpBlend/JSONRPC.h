#pragma once
#include "TCP.h"
#include "JSON.h"
#include <queue>
#include <map>
#include <memory>
#include <functional>

class JSONRPCReactor
{
public:
	struct Handler
	{};

	//struct Request
	//{
	//	uint64_t id;
	//	std::weak_ptr<TCPConnection> conn;
	//	Request() : id( 0 ) {}
	//	Request( uint64_t _id, std::shared_ptr<TCPConnection> _conn ) : id( _id ), conn( _conn ) {}
	//};
	//struct Subscription : Request
	//{

	//};
	struct Method
	{
		typedef std::function<JSON::Object( JSON::Object const&, Handler* )> fn_t;
		Handler* ref;
		fn_t cb;
		Method() : ref(nullptr) {}
		Method( fn_t&& _cb, Handler* _ref = nullptr ) : cb{ std::move( _cb ) }, ref( _ref ) {}
		Method( Method&& ) = default;
		Method( Method const& ) = delete;
		Method& operator=( Method&& ) = default;
		Method& operator=( Method const& ) = delete;
	};

protected:
	//std::map<std::u8string, std::vector<Subscription>> subscriptions;
	std::map<std::u8string, Method> methods;
	std::queue<JSON> requests;
	std::queue<JSON> responses;

	uint64_t currID = 1;
	// singleton pattern
	JSONRPCReactor() {}
	static std::unique_ptr<JSONRPCReactor> _;

public:
	// only move constructible/copyable 
	JSONRPCReactor( JSONRPCReactor const& ) = delete;
	JSONRPCReactor& operator=( JSONRPCReactor const& ) = delete;
	JSONRPCReactor( JSONRPCReactor&& ) = default;
	JSONRPCReactor& operator=( JSONRPCReactor&& ) = default;

	~JSONRPCReactor() {}

	static bool create()
	{
		_.reset( new JSONRPCReactor );
		return _.get() != nullptr;
	}
	static void destroy()
	{
		if( _.get() )
			_.release();
	}
	inline static JSONRPCReactor& get() { if( _.get() ) return *_.get(); else throw std::exception( "JSONRPCReactor not initialized." ); }
	inline static bool registerMethod( std::u8string const& name, Method::fn_t&& _cb, Handler* _ref = nullptr ) { return _->methods.emplace( name, Method( std::move( _cb ), _ref ) ).second; }
	//inline static bool notifyChange( std::u8string const& name, JSON::Value const& v ) 
	//{ 
	//	if( auto it = _->subscriptions.find( name ); it != _->subscriptions.end() )
	//	{
	//		JSON response( { { name, u8"stringvalue" } } );
	//		for( auto& s : it->second )
	//		{

	//		}
	//	}
	//	return false;
	//}
	inline static JSON handle( JSON const& request ) { 
		using namespace std;
		JSON response;
		try {
			if( auto iid = request.find( u8"id" ); request.end() != iid )
				response[u8"id"] = iid->second;
			else
				response[u8"id"] = 0;

			if( u8string const& a = request[u8"jsonrpc"]; a != u8"2.0" )
				throw exception( "invalid protocol. must be jsonrpc : \"2.0\"" );

			if( auto it = _->methods.find( request[u8"method"] ); it != _->methods.end() )
			{
				response[u8"result"] = it->second.cb( request[u8"params"].object, it->second.ref );
			}
			else throw exception( "unknown method" );
		}
		catch( std::exception& e )
		{
			response[u8"error"] = to_u8string( e.what() );
		}
		catch( ... )
		{
			response[u8"error"] = u8"internal server error";
		}
		return response;
	}
};

class JSONRPC : public TCPProto
{
protected:
	static ParserAdder _adder;
public:
	static ptr_t parse( TCPConnection& conn );
	static TYPE myType;
	JSONRPC( TCPConnection& conn ) : TCPProto( myType ) { state = STATE_ERROR; }
	JSONRPC( std::string const& cont ) : TCPProto( myType, cont ) {}
	virtual bool send( TCPConnection& conn );
};
