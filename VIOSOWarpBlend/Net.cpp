#include "Net.h"
#include <sstream>

using namespace std;
queue< VWBRemoteCommand > g_commandQueue;

int VWBTCPListener::heartBeatFn( void* param )
{
	while( m_tm != 0ms )
	{
		sendInfoTo( SocketAddress::broadcast( m_port ) );
		this_thread::sleep_for( m_tm );
	}
	return 0;
}

VWBTCPListener::VWBTCPListener( SocketAddress& s, std::chrono::duration<u_short, std::milli > tm )
: TCPListener( s )
, m_port( s.getPort() ) 
, m_tm( tm )
, m_lck()
{
	if( m_tm != 0ms )
	{
		m_heardBeatTh = thread( &VWBTCPListener::heartBeatFn, this, nullptr );
	}
}

VWBTCPListener::~VWBTCPListener()
{
	if( m_tm != 0ms && m_heardBeatTh.joinable() )
	{
		m_tm = 0ms;
		m_heardBeatTh.join();
	}
}

VWB_ERROR VWBTCPListener::add( VWB_Warper* pWarper )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	if( 0 == pWarper->port )
		return VWB_ERROR_PARAMETER;

	if( m_port != pWarper->port )
		return VWB_ERROR_FALSE;

	lock_guard sl( m_lck );
	for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
	{
		if( *it == pWarper )
		{
			logStr( 0, "ERROR: VWBTCPListen: duplicate warper!\n" );
			return VWB_ERROR_GENERIC;
		}
		if( 0 == strcmp( (*it)->channel, pWarper->channel ) )
		{
			logStr( 0, "ERROR: VWBTCPListen: duplicate warper name!\n" );
			return VWB_ERROR_GENERIC;
		}
	}
	m_warpers.push_back( pWarper );
	logStr( 2, "INFO: VWBTCPListen(%hu): warper \"%s\" added.\n", m_port, pWarper->channel );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWBTCPListener::remove( VWB_Warper* pWarper )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	//if( 0 == pWarper->port ) /// don't be over-picky ;)
	//	return VWB_ERROR_PARAMETER;

	//if( m_port != Socket::hton(pWarper->port) )
	//	return VWB_ERROR_FALSE;

	lock_guard sl( m_lck );
	for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
	{
		if( *it == pWarper )
		{
			logStr( 2, "INFO: VWBUDPListener(%hu): warper %s removed!\n", m_port, pWarper->channel );
			m_warpers.erase( it );
			return VWB_ERROR_NONE;
		}
	}
	return VWB_ERROR_FALSE;
}

VWB_ERROR VWBTCPListener::sendInfoTo( SocketAddress sa, SocketAddress* local )
{
	if( 0 == sa.sin_addr.s_addr )
		return VWB_ERROR_PARAMETER;
	if( 0xFFFFFFFF == sa.sin_addr.s_addr )
		return VWB_ERROR_PARAMETER;
	SocketAddress my;
	if( NULL == local )
	{
		vector<in_addr> list = Socket::getLocalIPList();
		my = SocketAddress( list[0].s_addr, sa.getPort() );
		local = &my;
	}
	ostringstream buf;
	char buff[20];
	try {
		buf << "VIOSOWarpBlend API " << VWB_Version_MAJ << "." << VWB_Version_MIN << "." << VWB_Version_MAI << "." << VWB_Version_REV << m_warpers.size() << " display(s) on " << local->getDottedDecimal(buff) <<":" << local->getPort() << "\015\012";
		lock_guard sl( m_lck );
		for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
		{
			VWB_Warper_base* p = (VWB_Warper_base*)*it;
			buf << "\"" << p->channel << "\"" << p->getMappingSize().cx << "x" << p->getMappingSize().cy << ".\015\012";
		}
		
		if( int(buf.tellp()) != Socket::sendDatagram( buf.str().c_str(), int(buf.tellp()), sa, true ) ) // sendto will always send all if smaller than SO_RECVBUF
			return VWB_ERROR_NETWORK;
	}catch( ... )
	{
		return VWB_ERROR_GENERIC;
	}
	return VWB_ERROR_NONE;
}

VWB_Warper* VWBTCPListener::getWarper( char const* szName )
{
	lock_guard sl( m_lck );
	for( auto it = m_warpers.begin(); it != m_warpers.end(); it ++ )
		if( (*it) && 0 == strcmp( (*it)->channel, szName ) )
			return *it;
	return NULL;
}

int VWBTCPListener::cbRead( Server* pServer )
{
	// a new connection has been established
	// we need to promote the new connection to own type to process data to make it call our virtuals
	m_peers.back().s = make_shared<VWBTCPConnection>(m_peers.back().s->detach(), m_peers.back().sa, this, pServer);
	return 0;
}

int VWBTCPListener::cbError( Server* pServer, int err )
{
	return 0;
}

VWBTCPConnection::VWBTCPConnection( Socket const& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer )
: TCPConnection( other, peerAddress, pL, pServer )
, m_pWarper( NULL )
, m_state(0)
, m_iCalls(0)
{
}

int VWBTCPConnection::readRequest()
{
	return 0;
}

int VWBTCPConnection::sendResponse()
{
	return 0;
}

int VWBTCPConnection::cbRead( Server* pServer )
{ // this is always called from within lock
	m_iCalls++;
	auto req = Request::parse( *this );
	auto state = m_req.front().get()->getState();
	if( Request::STATE_ERROR == state )
	{
		// report error
		logStr( 2, "INFO: No TCP request parsing failed\n" );
		return SOCKET_ERROR;
	}
	if( HttpRequest::STATE_PARSED == state )
	{
		logStr( 2, "INFO: Net receive request %s\n", m_req.front()->getRequest().c_str() );
		m_req.emplace( req );
		return 1;
	}
	return 0;
}

int VWBTCPConnection::cbError( Server* pServer, int err )
{
	return err;
}
