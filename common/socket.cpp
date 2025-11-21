// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "Platform.h"
#include "socket.h"
#include "logging.h"
#ifndef WIN32
#include <netdb.h>
#define FAR
#endif
using namespace std;
// ----------------------------------------------------------------------------------
//                               SocketAddress
// ----------------------------------------------------------------------------------

SocketAddress SocketAddress::gethostbyname( const std::string& url, unsigned short port )
{
	auto pos = url.find( ':' );
	if( !port )
	{
		if( string::npos != pos )
		{
			try {
				port = ( unsigned short )std::stoi( url.substr( pos + 1 ) );
			}
			catch( invalid_argument& ) {}
		}
	}
	string host = string::npos != pos ? url.substr( 0, pos ) : url;
	addrinfo* pai = nullptr;
	addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	auto err = getaddrinfo( host.c_str(), nullptr, &hints, &pai );
	if( 0 == err && pai )
	{
		sockaddr_in sTmp = *( struct sockaddr_in* )pai->ai_addr;
		sTmp.sin_port = Socket::hton( port );
		freeaddrinfo( pai );
		return sTmp;
	}
	else
	{
		return SocketAddress();
	}
}

// ----------------------------------------------------------------------------------
//                               Socket
// ----------------------------------------------------------------------------------

int Socket::create(int type, bool broadcast, int protocol, bool keepAlive,u_int qRecvBuffer,u_int qSendBuffer)
{ 
	int err;

	sock=socket( AF_INET, type, protocol);
		
	if(sock==INVALID_SOCKET)
        return lastNetError;

	if(broadcast)
	{
		if( (SOCK_DGRAM==type) && ( (sock!=SOCKET_ERROR) || (sock!=INVALID_SOCKET) ) )
		{
            int param=1;
            int err=::setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (const char*)&param, sizeof(int));
			if(err==SOCKET_ERROR) 
				return err;
		} 
		else 
			return SOCKET_ERROR;
	}

	if(keepAlive)
	{
		u_int param=1;
			
		err=::setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&param, sizeof(u_int));
		if(err)
			return err;
	}

	if(qRecvBuffer)
	{
		int sz=0;
		if( !getRecvBuffSize(sz) && (sz<(int)qRecvBuffer))
		{
			sz=(int)qRecvBuffer;
            err=::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char const FAR*)&sz, sizeof(sz));
			if(err)
				return err;
		}
	}

	if(qSendBuffer)
	{
		int sz=0;
		if( !getSendBuffSize(sz) && (sz<(int)qSendBuffer))
		{
			sz=(int)qSendBuffer;
            err=::setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char const FAR*)&sz, sizeof(sz));
			if(err)
				return err;
		}
	}

	return 0;
}

int Socket::setRecvBuff(int size)
{
	return ::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (const char FAR*)&size, sizeof(size));
}

int Socket::getRecvBuffSize(int& size)
{
    socklen_t len=sizeof(size);
	int err=::getsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char FAR*)&size, &len);

	if( err || (size<0))
	{
		size=-1;
		return SOCKET_ERROR;
	}

	return 0;
}

int Socket::getSendBuffSize(int& size)
{
    socklen_t len=sizeof(size);
	int err=::getsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char FAR*)&size, &len);

	if( err || (size<0))
	{
		size=-1;
		return SOCKET_ERROR;
	}

	return 0;
}

int Socket::close()
{
    if(sock == 0 || sock == INVALID_SOCKET)
		return SOCKET_ERROR;
	int ret=0;

#ifdef WIN32
	ret=::closesocket(sock);
#else  /* def WIN32 */
	ret=::close(sock);
#endif /* def WIN32 */
	sock=0;

	return ret;
}

int Socket::tryRead(const double timeout) const 
{
	FD_SET fd;
	FD_ZERO( &fd );
	FD_SET( sock, &fd );
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,&fd,NULL,NULL,pt);
}

int Socket::tryWrite(const double timeout) const 
{
	FD_SET fd;
	FD_ZERO( &fd );
	FD_SET( sock, &fd );
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,NULL,&fd,NULL,pt);
}

int Socket::test(const double timeout) const
{
	FD_SET fd;
	FD_ZERO( &fd );
	FD_SET( sock, &fd );
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,NULL,NULL,&fd,pt);
}

int	Socket::send(const char* pch, int size, const double timeout)
{
	if( 1 == tryWrite( timeout ) )
		return sendDirect( pch, size );

	return SOCKET_ERROR;
}

int	Socket::sendDirect(const char* pch, int size)
{
	int bytesSent = 0;

	do // as it is OK to send an empty packet, we need to run the loop always once
	{
		int bytesThisTime = ::send( sock, pch, size, 0 );
		if( SOCKET_ERROR == bytesThisTime ) 
			return SOCKET_ERROR;
		size -= bytesThisTime;
		bytesSent += bytesThisTime;
		pch += bytesThisTime;
	} while( 0 != size );
	return bytesSent;
}

int	Socket::recv(char* pch, int iSize, const double timeout)
{ 
	if(tryRead(timeout)==1) 
	{
		return recvDirect( pch, iSize );
	} 

	return 0;
}

int	Socket::recvDirect(char* pch, int size)
{ 
	int bytesRec = 0;

	do // as it is OK to get an empty packet, we need to run the loop always once
	{
		int bytesThisTime = ::recv( sock, pch, size, 0 );
		if( SOCKET_ERROR == bytesThisTime )
			return SOCKET_ERROR;
		size -= bytesThisTime;
		bytesRec += bytesThisTime;
		pch += bytesThisTime;
	} while( 0 != size );
	return bytesRec;
}


SocketAddress	Socket::getpeeraddr()
{ 
	sockaddr a;
    socklen_t addLen=sizeof(sockaddr);
	if(::getpeername(sock,&a,&addLen) == SOCKET_ERROR)
		a.sa_family=AF_UNSPEC;
	return a;
}

SocketAddress   Socket::getsockaddr() 
{
	sockaddr a;
    socklen_t addLen=sizeof(sockaddr);

    memset( &a, 0, addLen);
	if(::getsockname( sock, &a, &addLen)==SOCKET_ERROR)
		a.sa_family=AF_UNSPEC;

	return a;
}

int Socket::getsockaddr(sockaddr_in& addr,socklen_t& nAddr)
{
    nAddr=sizeof(sockaddr_in);
    memset( &addr, 0, nAddr);
    return ::getsockname( sock, (sockaddr*)&addr, &nAddr);
}

int Socket::sendDatagram(const char* pch,const int iSize,SocketAddress const& sa, bool dontRoute)
{
	//fd_set fd={1,sock};

	//if(::select( 0, NULL, &fd, NULL, NULL)==1)
        return ::sendto( sock, pch, iSize,(dontRoute)? MSG_DONTROUTE : 0, sa, sizeof(sockaddr_in));

	//return 0;
}

int Socket::recvDatagram(char* pch,const int iSize, SocketAddress* sa, const double timeout )
{
	FD_SET fd;
	FD_ZERO( &fd );
	FD_SET( sock, &fd );
	socklen_t size=sizeof(SocketAddress);
	int res;
	if( timeout != INFINITETIMEOUT )
	{
		timeval to{ long( timeout ), long( ( timeout - long( timeout ) ) * 1000000 ) };
		res = ::select( 0, &fd, NULL, NULL, &to );
	}
	else
	{
		res = ::select( 0, &fd, NULL, NULL, nullptr );
	}

	if( (res!=SOCKET_ERROR) && (res>0) )
	{
		res= ::recvfrom( sock, pch, iSize, 0, (struct sockaddr*)sa, &size);
	}

	return res;
}

//static
int Socket::close(SOCKET& s)
{ 
	if( !s || (s==INVALID_SOCKET) )
		return 0;

	int ret;

#ifdef WIN32
	ret=::closesocket(s);
#else  /* def WIN32 */
	ret=::close(s);
#endif /* def WIN32 */

    s=0;

	return ret;
}

//static
std::string Socket::gethostname()
{
    std::string s(257,'\0');
    ::gethostname( (char*)s.data(), 256 );
    s.erase( strlen(s.c_str()) );
	return s;
}

//static
std::vector<in_addr> Socket::getLocalIPList()
{
	std::vector<in_addr> l;
    static const in_addr lh = makeInAddr(0x0100007F);

	std::string s( 256, 0 );
	if( 0 == ::gethostname( &s[0], 256 ) )
	{
		addrinfo* pai = NULL;
		char portStr[6]; _itoa_s( 80, portStr, 10 );
		portStr[5] = 0;
		addrinfo hints = { 0 };
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		if( 0 == getaddrinfo( s.c_str(), portStr, &hints, &pai ) && pai )
		{
			for( addrinfo* p = pai; p; p = p->ai_next )
			{
				if( AF_INET == p->ai_family && 4 >= p->ai_addrlen )
				{
					l.push_back((( struct sockaddr_in* )p->ai_addr)->sin_addr);
				}
			}
			freeaddrinfo( pai );
		}
	}
	l.push_back( lh );
	return l;
}

//static
std::string	Socket::gethostbyaddr(const SocketAddress& sa)
{ 
	string name( NI_MAXHOST, '\0' );
	auto err = getnameinfo( sa, sizeof( sa ), name.data(), ( unsigned int )name.length(), NULL, 0, NI_NUMERICSERV );
    if( err ) 
		name.clear(); // set empty
	else
		name.erase( name.find( '\0' ) ); // trim
	return name;
}

///////////////////////////////////////////////////////////////////////////////////////////

SockIn::SockIn( int protocol, SocketAddress const& sa )
{
	if( IPPROTO_TCP == protocol )
	{
		create( SOCK_STREAM, false, IPPROTO_TCP, true, 0, 0 );
	}
	else if( IPPROTO_UDP == protocol )
	{
		create( SOCK_DGRAM, true, IPPROTO_UDP, false, 0, 0 );
	}
	else
		throw -1;
	bind( sa );
}

///////////////////////////////////////////////////////////////////////////////////////////

Server::Server()
	: m_modalState( 0 )
	, m_sto{ 0,16000 }
{
	FD_ZERO( &m_readers );
	FD_ZERO( &m_writers );
	ifStartSockets()
	{
	}
}

Server::Server( shared_ptr<SockIn> const& s, bool bRunModal )
	: m_modalState( 0 )
	, m_sto{ 0,16000 }
{
	FD_ZERO( &m_readers );
	FD_ZERO( &m_writers );
	ifStartSockets()
	{
		addReceiver( s );
		if( bRunModal )
			doModal();
	}
}

Server::~Server()
{
	VWB_LockedStatement( m_mtxGlobal )
	{
		m_listeners.clear();
	}
	if( m_thread.joinable() )
	{
		m_modalState = 0;
		m_thread.join();
	}
	closeSockets();
}

int Server::addReceiver( shared_ptr<SockIn> s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			m_listeners.push_back( s );
			FD_SET( *s, &m_readers );
			logStr( 2, "INFO: Server added socket %08X. Server now listens to %u connections.\n", ( SOCKET )*s, ( u_int )m_listeners.size() );
			return 0;
		}
	}
	logStr( 0, "Server::add receiver error.\n" );
	return SOCKET_ERROR;
}

int Server::removeReceiver( shared_ptr<SockIn> s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			for( Listeners::iterator it = m_listeners.begin(); it != m_listeners.end(); it++ )
				if( *it == s )
				{
					FD_CLR( **it, &m_readers );
					m_listeners.erase( it );
					logStr( 2, "Server removed socket %08X. Server now listens to %u connections.\n", ( SOCKET )*s, ( u_int )m_listeners.size() );
					return 0;
				}
		}
	}
	logStr( 0, "Server::remove receiver error.\n" );
	return SOCKET_ERROR;
}

int Server::addResponder( Socket& s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			FD_SET( s, &m_writers );
			return 0;
		}
	}
	logStr( 0, "Server::add responder error.\n" );
	return SOCKET_ERROR;
}

int Server::removeResponder( Socket& s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			FD_CLR( s, &m_writers );
			return 0;
		}
	}
	logStr( 0, "Server::remove responder error.\n" );
	return SOCKET_ERROR;
}

int Server::doModalStep()
{
	int n = 0;
	timeval to = m_sto;
	FD_SET r;
    FD_SET w;
    if( FD_IS_ANY_SET( m_readers ) ) // there can't be any writers without readers in TCP
	{
		Listeners tmp;
		VWB_LockedStatement( m_mtxGlobal )
		{
			tmp = m_listeners;
			r = m_readers;
			w = m_writers;
		}
	else
	{
		logStr( 0, "Server: Could not aquire mutex. Server is stopping.\n" );
		return -2;
	}
	n = ::select( ( int )m_listeners.size(), &r, &w, NULL, &to );
	if( 0 < n )
	{
		// handle readers
		for( Listeners::iterator it = tmp.begin(); it != tmp.end(); it++ )
		{
			if( FD_ISSET( **it, &r ) )
			{
				n = ( *it )->processRead( this );
			}
		}
		// handle writers
		for( Listeners::iterator it = tmp.begin(); it != tmp.end(); it++ )
		{
			if( FD_ISSET( **it, &w ) )
			{
				( *it )->processWrite( this ); // handle all ready writing sockets
			}
		}
	}
	else if( SOCKET_ERROR == n )
	{
		int err = lastNetError;
		logStr( 0, "Server: Socket Error %i. Server is stopping.\n", err );
	}
	}
	else if( MODALSTATE_RUN == m_modalState )
	{ // idle
		sleep( m_sto.tv_usec / 1000 );
	}
	return n;
}

int Server::endModal( int code )
{
	if( MODALSTATE_RUN == m_modalState )
	{
		m_modalState = code;
		if( m_thread.joinable() )
		{
			m_thread.join();
		}
	}
	return m_modalState;
}

int Server::doModal()
{
	if( m_thread.joinable() )
	{
		if( MODALSTATE_RUN == m_modalState )
		{
			return -2;
		}
		else
		{
			m_thread.join();
		}
	}
	m_thread = std::thread( &Server::modalLoop, this );
	if( m_thread.joinable() )
		return 0;
	logStr( 0, "Server: FATAL ERROR cannot sart listener loop.\n" );
	return SOCKET_ERROR;
}

u_int Server::modalLoop()
{
	try
	{
		m_modalState = MODALSTATE_RUN;
		logStr( 1, "Server: Server starts listening.\n" );
		while( MODALSTATE_RUN == m_modalState )
		{
			if( 0 > doModalStep() )
				m_modalState = MODALSTATE_ERR;
		}
		logStr( 1, "Server: Server stops listening.\n" );
		return 0;
	}
	catch( ... )
	{
		m_modalState = MODALSTATE_EXC;
		return SOCKET_ERROR;
	}
}

bool Server::isRunningModal()
{
	return MODALSTATE_RUN == m_modalState;
}
