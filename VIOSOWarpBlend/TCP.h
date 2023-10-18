#pragma once
#include "socket.h"

// Forwards
class TCPConnection;

////////////////////////////////////////////////////////////////////
// The listener spawns a TCPConnection at a connect
class TCPListener : public SockIn
{
public:
	typedef struct Peer { void* pData{}; SocketAddress sa; std::shared_ptr<TCPConnection> s; } Peer;
	typedef std::vector< Peer > PeerList;
protected:
	PeerList m_peers;

	TCPListener( TCPListener&& other ) : SockIn( std::move(other) ) {}
	TCPListener() : SockIn() {}
public:
	int size() { ( int )m_peers.size(); }
	Peer& getPeer( int i ) { return m_peers[i]; }
	Peer const& getPeer( int i ) const { return m_peers[i]; }
	int getPeerIndex( TCPConnection const* pC ) const;
	void closePeer( int i ) { m_peers.erase( m_peers.begin() + i ); }

	TCPListener( SocketAddress const& sa );

private:
	virtual int processRead( Server* pServer );
};


////////////////////////////////////////////////////////////////////
// A TCP connection
class TCPConnection : public SockIn
{
public:
	typedef std::list< std::string > DataStream;
protected:
	DataStream m_strIn;
	DataStream m_strOut;
	int m_iRcvBuffSize;
	int m_iSndBuffSize;
	int m_iReadOffs;
	int m_iReadSize;
	bool m_bPendingSend;
	SocketAddress m_peerAddr;
	TCPListener* m_pListener;
	Server* m_pServer;
	mutable std::mutex m_mtxRWIn;
	mutable std::mutex m_mtxRWOut;
	timeval m_sto;

public:
	TCPConnection( SockIn&& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer );
	TCPConnection( SocketAddress connectTo );
	virtual ~TCPConnection();

	int write( char const* buff, int size );

	// returns number of read chars, obtain available number of chars in stream from getNumRead()
	// returns SOCKET_ERROR if buffer too small
	int read( char* buff, int iBuffSize, int size );

	// @param buff			the buffer to read into
	// @param iBuffSize		the size of the buffer to read until
	// @param szDelimiter	optional zero terminated string, set to network new-line "\015\012" 
	// if buff is NULL, it returns the needed size including terminating zero char
	// returns number of read chars
	// returns 0 if no line is found or an empty line is read
	// returns SOCKET_ERROR if buffer too small
	int readUntil( char* buff, int iBuffSize, char const* szDelimiter = NULL ); // Adds zero termination to string

	int cmpFront( char const* str ) const;
	int processAsClient();

	bool isWritePending() const { return m_bPendingSend; }
	int getNumRead() const { return m_iReadSize; }
	int getCurrRcvBuffSize() const { return m_iRcvBuffSize; }
	int getCurrSndBuffSize() const { return m_iSndBuffSize; }
private:
	virtual int processRead( Server* pServer );
	virtual int processWrite( Server* pServer );
	virtual int processError( Server* pServer );
};

////////////////////////////////////////////////////////////////////
// the client opens a TCP connection
class TCPClient
{
public:
	TCPClient( SocketAddress sa );
	virtual ~TCPClient();
};

//////////////////////////////////////////////////////////////
// Base class of all TCP protocols
// the idea is to either receive a request or a response via network or to create from parameter
// Server: Listen -> Request( conn ) -> Response( params ) -> Response.send()
// Client: Request( params ) -> Request.send() -> Response( conn )
class TCPProto
{
public:
	typedef std::shared_ptr<TCPProto> ptr_t;
	typedef ptr_t( *ParserFN )( TCPConnection& conn );
protected:
	static ParserFN _parserFNs[10];
public:
	class ParserAdder {
	public:
		ParserAdder( ParserFN parser ) {
			for( auto i = 0; i != ARRAYSIZE( _parserFNs ); i++ )
			{
				if( nullptr == _parserFNs[i] )
				{
					_parserFNs[i] = parser;
					return;
				}
			}
		}
	};
public:
	typedef enum STATE {
		STATE_UNDEF,
		STATE_INIT,
		STATE_PARSED = 0x7FFFFFFF,
		STATE_ERROR = 0xFFFFFFFF
	} STATE;

	typedef union TYPE {
		uint64_t id;
		char txt[8];
		bool operator==( char const* ch ) const {
			for( int i = 0; i != 8; i++ ) {
				if( 0 == txt[i] && 0 == ch[i] )
					return true;
				if( txt[i] != ch[i] )
					return false;
			}
			return true;
		}
		bool operator==( TYPE const& t ) const {
			return operator==( txt );
		}
		TYPE( char const* ch ) noexcept {
			strncpy( txt, ch, 8 );
		}
		TYPE( uint64_t x ) noexcept {
			id = x;
		}
	} TYPE;
	static TYPE TYPE_UNDEF;

	typedef int State;
protected:
	TYPE type;
	State state;
	std::string		content;

public:
	static ptr_t parse( TCPConnection& conn );
	
	TCPProto() noexcept
		: type( TYPE_UNDEF )
		, state( STATE_UNDEF )
	{};
	TCPProto( TYPE t, std::string const& cont = std::string() ) : type( t ), state( cont.empty() ? STATE_UNDEF : STATE_INIT ), content( cont ) {}
	TCPProto( TCPProto const& other ) noexcept
		: type( other.type )
		, state( other.state )
		, content( other.content )
	{};
	TCPProto( TCPProto&& other ) noexcept
		: type( other.type )
		, state( other.state )
		, content( std::move(other.content))
	{
		other.state = STATE_UNDEF;
		other.type = TYPE_UNDEF;
	};
	State getState() const { return state; }
	TYPE getType() const { return type; }
	std::string const& getContent() const { return content; }
	//TCPProto& operator=( TCPProto const& other ) noexcept {
	//	type = other.type;
	//	state = other.state;
	//	return *this;
	//}
	virtual bool send( TCPConnection& conn ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
// a command is simply a string, can act as request and response
class Command : public TCPProto
{
public:
	Command( TCPConnection& conn );
	Command( std::string const& content );
	virtual bool send( TCPConnection& conn );
};
