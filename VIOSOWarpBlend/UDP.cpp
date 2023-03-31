#include "Platform.h"
#include "UDP.h"
#include "logging.h"
using namespace std;

UDPListener::UDPListener( SocketAddress& sa )
	: SockIn( IPPROTO_UDP, sa )
	, m_szRcvBuff( 0 )
{
	getRecvBuffSize( m_szRcvBuff );
	char buff[20];
	logStr( 1, "INFO: New UDPListener(%s:%hu).\n", sa.getDottedDecimal( buff ), sa.getPort() );
}

int UDPListener::processRead( Server* pServer )
{
	DataPackage a( std::string( m_szRcvBuff, 0 ), SocketAddress() );
	int size = ( int )sizeof( SocketAddress );
	int r = ::recvfrom( sock, a.first.data(), ( int )a.first.size(), 0, ( struct sockaddr* )&a.second, &size );
	if( 0 < r )
	{
		a.first.resize( r );
		a.first.shrink_to_fit();
		m_received.push( a );
		return cbRead( pServer );
	}
	else
		return cbError( pServer, r );
}
