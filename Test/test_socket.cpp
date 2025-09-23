#include "socket.h"
//#include "TCP.h"
//#include "UDP.h"
//#include "HTTP.h"
//#include "JSONRPC.h"

bool test_socket()
{
	using namespace std;

	WSADATA wsad{};
	WSAStartup( MAKEWORD( 2, 2 ), &wsad );

	auto sa = Socket::gethostbyname( "ANYSTATION:3000" );
	auto sa2 = SocketAddress( "ANYSTATION:3000" );
	auto ss = Socket::gethostbyaddr( SocketAddress( "ANYSTATION" ) );

	//bool isSecure;
	//string host;
	//uint16_t port;
	//string path;
	//HttpBase::ParamMap params;
	//string fragment;
	//int ok;
	//ok = HttpBase::resolveURL((char const*)u8"hp://sasd.asd.co", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"https://sasd.asd.co", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd:81.asd.co", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd.asd.co:818#anc", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd.asd.co:888?bla=blubb&cc=abra cadabra!", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd.asd.co#bla=blubb&cc=abra cadabra!", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd.asd.co/bla.htm#bla=blubb&cc=abra cadabra!", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://sasd.asd.co/bla.htmbla=blubb&cc=abra cadabra!#anc", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"https://a.b?sdasd~/tr++/üü&?sdf#sadf", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"http://übers.na.bend/sdasd~/tr++/üü&?sdf#sadf", isSecure, host, port, path, params, fragment);
	//ok = HttpBase::resolveURL((char const*)u8"sdasd~/tr++/üü&?sdf#sadf", isSecure, host, port, path, params, fragment);
	//auto list = Socket::getLocalIPList();
	//for (auto const& ip : list)
	//{
	//	in_addr a = ip;
	//}
	WSACleanup();
	return true;
}