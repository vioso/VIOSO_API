#include "Platform.h"
#include "TCP.h"
#include "logging.h"

using namespace std;

TCPProto::ParserFN TCPProto::_parserFNs[10]{};
TCPProto::TYPE TCPProto::TYPE_UNDEF("");


TCPListener::TCPListener( SocketAddress const& sa )
	: SockIn( IPPROTO_TCP, sa )
{
	listen();
	char buff[20];
	logStr( 1, "INFO: New TCPListener(%s:%hu).\n", sa.getDottedDecimal( buff ), sa.getPort() );
}

int TCPListener::processRead( Server* pServer )
{
	Peer peer;
	peer.pData = NULL;
	peer.s = make_shared<TCPConnection>( Socket::accept( peer.sa ), peer.sa, this, pServer );
	if( 0 < *peer.s )
	{
		m_peers.push_back( peer );
		cbRead( pServer );
		pServer->addReceiver( m_peers.back().s );
		return 0;
	}
	else
		cbError( pServer, ( int )( SOCKET )*peer.s );
	return SOCKET_ERROR;
}

int TCPListener::getPeerIndex( TCPConnection const* pC ) const
{
	for( PeerList::const_iterator it = m_peers.begin(); it != m_peers.end(); it++ )
	{
		if( pC == it->s.get() )
			return ( int )( it - m_peers.begin() );
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////

TCPConnection::TCPConnection( SockIn&& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer )
	: SockIn( std::move(other) )
	, m_iRcvBuffSize( 0 )
	, m_iSndBuffSize( 0 )
	, m_iReadOffs( 0 )
	, m_peerAddr( peerAddress )
	, m_pListener( pL )
	, m_pServer( pServer )
	, m_bPendingSend( false )
	, m_iReadSize( 0 )
	, m_sto()
{
	getRecvBuffSize( m_iRcvBuffSize );
	getSendBuffSize( m_iSndBuffSize );
	m_sto.tv_sec = 0;
	m_sto.tv_usec = 16000;
	char buff[20];
	logStr( 2, "Info: TCPConnection accepted at server from %s:%hu.\n\tBuffers in:%i out:%i.\n", peerAddress.getDottedDecimal( buff ), peerAddress.getPort(), m_iRcvBuffSize, m_iSndBuffSize );
}

TCPConnection::TCPConnection( SocketAddress connectTo ) // client connection
	: SockIn( IPPROTO_TCP, connectTo )
	, m_iRcvBuffSize( 0 )
	, m_iSndBuffSize( 0 )
	, m_iReadOffs( 0 )
	, m_peerAddr( connectTo )
	, m_pListener( NULL )
	, m_pServer( NULL )
	, m_bPendingSend( false )
	, m_iReadSize( 0 )
	, m_sto()
{
	char buff[20];
	if( 0 != connect( connectTo ) )
	{
		logStr( 0, "Error: TCPConnection client failed to connect to server at %s:%hu.. Reason %i.\n", connectTo.getDottedDecimal( buff ), connectTo.getPort(), lastNetError );
		return;
	}
	getRecvBuffSize( m_iRcvBuffSize );
	getSendBuffSize( m_iSndBuffSize );
	m_sto.tv_sec = 0;
	m_sto.tv_usec = 16000;
	logStr( 2, "Info: TCPConnection client connected to server at %s:%hu as %08x.\n\tBuffers in:%i out:%i.\n", connectTo.getDottedDecimal( buff ), connectTo.getPort(), ( SOCKET )*this, m_iRcvBuffSize, m_iSndBuffSize );
}

TCPConnection::~TCPConnection()
{
	if( *this )
		logStr( 2, "Info: TCPConnection %08x closed.\n", ( SOCKET )*this );
}

int TCPConnection::write( char const* buff, int size )
{
	int r = 0;
	VWB_LockedStatement( m_mtxRWOut )
	{
		while( size > m_iSndBuffSize )
		{
			m_strOut.push_back( std::string( buff, m_iSndBuffSize ) );
			size -= m_iSndBuffSize;
			m_iReadSize += m_iSndBuffSize;
			buff += m_iSndBuffSize;
		}
		if( 0 != size )
		{
			m_strOut.push_back( std::string( buff, size ) );
		}
		if( !m_strOut.empty() )
		{
			r = sendDirect( &m_strOut.front()[0], ( int )m_strOut.front().size() );
			m_strOut.pop_front();
			if( m_pServer )
				m_pServer->addResponder( *this );
			m_bPendingSend = true;
		}
	}
	return r;
}

int TCPConnection::read( char* buff, int szBuff, int size )
{
	int r = 0;
	if( szBuff < size )
		return SOCKET_ERROR;
	VWB_LockedStatement( m_mtxRWIn )
	{
		while( !m_strIn.empty() && 0 < m_strIn.front().size() - m_iReadOffs )
		{
			if( 0 == size || 0 == szBuff )
				break;
			int ts = size > szBuff ? szBuff : size; // this is what fits into the string
			int tr = ( ( int )m_strIn.front().size() ) - m_iReadOffs; // this is what will be read in this run
			if( tr <= ts )
			{ // all fits in
				memcpy( buff, &m_strIn.front()[m_iReadOffs], tr );
				m_iReadOffs = 0;
				m_strIn.pop_front();
				size -= tr;
				szBuff -= tr;
				r += tr;
				buff += tr;
			}
			else
			{ // only part fits in
				memcpy( buff, &m_strIn.front()[m_iReadOffs], ts );
				m_iReadOffs += ts;
				r += ts;
				buff += ts;
				break;
			}
		}
		m_iReadSize -= r;
	}
	return r;
}

int TCPConnection::readUntil( char* buff, int iBuffSize, const char* szDelimiter )
{
	int r = 0;
	if( NULL == szDelimiter || 0 == szDelimiter[0] )
		szDelimiter = "\015\012";
	const int iDelSize = ( int )strlen( szDelimiter );

	VWB_LockedStatement( m_mtxRWIn )
	{
		auto it = m_strIn.begin();
		char* pL = NULL;
		int nn = 0;
		if( it != m_strIn.end() )
		{
			char const* b = &( *it )[m_iReadOffs];
			char const* bE = b + ( int )( it->size() - m_iReadOffs );
			for( ; b < bE; b++ )
				if( *b == szDelimiter[nn] )
				{
					if( iDelSize == ++nn )
						break;
				}
				else
					nn = 0;

			if( iDelSize == nn )
			{
				r += ( int )( b - &( *it )[m_iReadOffs] ) + 1 - iDelSize;
			}
			else
				r += ( int )it->size() - m_iReadOffs;
		}
		int c = 0;
		if( iDelSize != nn )
		{
			for( it++; it != m_strIn.end(); it++ )
			{
				char const* b = &( *it )[0];
				char const* bE = b + ( int )it->size();
				for( ; b < bE; b++ )
					if( *b == szDelimiter[nn] )
					{
						if( iDelSize == ++nn )
							break;
					}
					else
						nn = 0;
				if( iDelSize == nn )
				{
					c = ( int )( b - &( *it )[0] ) + 1 - iDelSize;
					r += c;
					break;
				}
				r += ( int )it->size();
			}
		}

		if( m_strIn.end() == it )
			return 0; // not found

		if( NULL == buff )
			return r + 1;

		if( r > iBuffSize )
			return SOCKET_ERROR;

		int t = ( int )( m_strIn.front().size() - m_iReadOffs );
		int o = r;
		while( t < o )
		{
			memcpy( buff, &m_strIn.front()[m_iReadOffs], t );
			o -= t;
			buff += t;
			m_strIn.pop_front();
			m_iReadOffs = 0;
			m_iReadSize -= t;
			t = ( int )m_strIn.front().size();
		}
		if( 0 != o )
		{
			memcpy( buff, &m_strIn.front()[m_iReadOffs], o );
		}
		buff[o] = 0;
		m_iReadOffs += o + iDelSize; // this is ok as we found the delimiter here
		m_iReadSize -= o + iDelSize;
		if( m_iReadOffs > ( int )m_strIn.front().size() ) // if it exceeded current buffer, we move onward
		{
			m_iReadOffs -= ( int )m_strIn.size();
			m_strIn.pop_front();
		}
	}
	return r;
}


int TCPConnection::cmpFront( char const* str ) const
{
	VWB_LockedStatement( m_mtxRWIn )
	{
		if( strlen( str ) <= ( m_strIn.front().size() - m_iReadOffs ) )
		{
			return strcmp( str, &m_strIn.front()[m_iReadOffs] );
		}
	}
	return SOCKET_ERROR;
}

int TCPConnection::processAsClient()
{
	int n = 0;
	timeval to = m_sto;
	FD_SET r; FD_ZERO( &r );
	FD_SET( *this, &r );
	FD_SET w; FD_ZERO( &w );
	if( m_bPendingSend )
		FD_SET( *this, &w );
	FD_SET e; FD_ZERO( &e );
	n = ::select( 1, &r, &w, &e, &to );
	if( 0 < n )
	{
		if( FD_ISSET( *this, &r ) )
			n = processRead( NULL );
		if( FD_ISSET( *this, &w ) )
			n = processWrite( NULL );
		if( FD_ISSET( *this, &w ) )
			n = processError( NULL );
	}
	if( SOCKET_ERROR == n )
	{
		int err = lastNetError;
		logStr( 0, "TCPClient: Socket Error %i.\n", err );
		close();
	}
	return n;
}

int TCPConnection::processRead( Server* pServer )
{
	if( std::numeric_limits<int>::max() - m_iRcvBuffSize < m_iReadSize )
	{
		logStr( 0, "Error: Receiver buffer of socket %08x full.\nClosing connection.\n", ( SOCKET )*this );
		return -1;
	}
	std::string s( m_iRcvBuffSize, 0 );
	int r = recvDirect( &s[0], m_iRcvBuffSize );
	if( 0 < r )
	{
		VWB_LockedStatement( m_mtxRWIn )
		{
			s.resize( r );
			m_strIn.push_back( s );
			m_iReadSize += r;
		}
	else
	{
		r = SOCKET_ERROR;
		logStr( 0, "Error: Connection socket %08x read mutex inaccessible.\n", ( SOCKET )*this );
	}
	if( r != SOCKET_ERROR )
	{
		r = cbRead( pServer );
		return r;
	}
	}

	logStr( 1, "Closing socket %08x reason %i.\n", ( SOCKET )*this, r );
	r = cbError( pServer, r );
	if( m_pListener )
	{
		int i = m_pListener->getPeerIndex( this );
		if( 0 <= i )
		{
			if( pServer )
				pServer->removeReceiver( m_pListener->getPeer( i ).s );
			m_pListener->closePeer( i );
		}
	}
	return r;
}

int TCPConnection::processWrite( Server* pServer )
{
	int r = 0;
	VWB_LockedStatement( m_mtxRWOut )
	{
		if( m_strOut.empty() )
		{
			m_bPendingSend = false;
			if( pServer )
				pServer->removeResponder( *this );
		}
		else
		{
			r = sendDirect( &m_strOut.front()[0], ( int )m_strOut.front().size() );
			m_strOut.pop_front();
		}
		return r;
	}

	r = SOCKET_ERROR;
	logStr( 0, "Error: Connection socket %08x read write inaccessible.\n", ( SOCKET )*this );
	logStr( 1, "Closing socket %08x reason %i.\n", ( SOCKET )*this, r );
	return r;
}

int TCPConnection::processError( Server* pServer )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Client

TCPClient::TCPClient( SocketAddress sa )
{
}

TCPClient::~TCPClient()
{
}

///////////////////////////////////////////////////////////////////////////////////////////
// Request

TCPProto::ptr_t TCPProto::parse( TCPConnection& conn )
{
	ptr_t r;
	for( auto i = 0; i != ARRAYSIZE( _parserFNs ); i++ )
	{
		r = (_parserFNs[i])( conn );
		if( r )
			return r;
	}
	return make_shared<Command>( conn ); // all other parsers failed, we create a command string proto
}

Command::Command( TCPConnection& conn ) : TCPProto( "cmd" )
{
	
}

Command::Command( std::string const& content ) : TCPProto( "cmd", content )
{
	// a command must be terminated with a /15/10

}

///////////////////////////////////////////////////////////////////////////////////////////
//
bool Command::send( TCPConnection& conn )
{
	conn.send( content.data(), (int)content.size() ); // TODO: upgrade from int to ssize_t
	return false;
}

