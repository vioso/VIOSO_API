#include "socket.h"

class UDPListener : public SockIn
{
public:
	typedef std::pair< std::string, SocketAddress > DataPackage;
	typedef std::queue< DataPackage > RecvQueue;
protected:
	RecvQueue m_received;
	int m_szRcvBuff;

	UDPListener( UDPListener const& other ) : SockIn( other ), m_szRcvBuff( 0 ) {}
	UDPListener() : SockIn(), m_szRcvBuff( 0 ) {}
public:

	DataPackage& front() { return m_received.front(); }
	int size() { ( int )m_received.size(); }
	void pop() { m_received.pop(); }

	UDPListener( SocketAddress& sa );

private:
	virtual int processRead( Server* pServer );
};

