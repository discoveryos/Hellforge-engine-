#include "../../hflib/hfprecompiled.h"
#include "hflinux_local.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>

hfCVar net_ip( "net_ip", "localhost", 0, "local IP address" );

bool NET_StringToSockaddr( const char *s, sockaddr_in *sadr, bool doDNSResolve );
void Net_NetadrToSockadr( const netadr_t *a, sockaddr_in *s );
void Net_SockadrToNetadr( sockaddr_in *s, netadr_t *a );

/*
================
NET_ErrorString
================
*/
char *NET_ErrorString() {
	static char buf[128];
	hfStr::snPrintf( buf, sizeof( buf ), "%s (%d)", strerror( errno ), errno );
	return buf;
}

/*
================
Net_NetadrToSockadr
================
*/
void Net_NetadrToSockadr( const netadr_t *a, sockaddr_in *s ) {
	memset( s, 0, sizeof( *s ) );
	s->sin_family = AF_INET;
	if ( a->type == NA_BROADCAST ) {
		s->sin_addr.s_addr = INADDR_BROADCAST;
	} else if ( a->type == NA_IP ) {
		s->sin_addr.s_addr = *(int *)&a->ip[0];
	}
	s->sin_port = htons( (short)a->port );
}

/*
================
Net_SockadrToNetadr
================
*/
void Net_SockadrToNetadr( sockaddr_in *s, netadr_t *a ) {
	if ( s->sin_addr.s_addr == INADDR_ANY || s->sin_addr.s_addr == INADDR_BROADCAST ) {
		a->type = NA_BROADCAST;
	} else {
		a->type = NA_IP;
	}
	a->port = ntohs( (short)s->sin_port );
	*(int *)&a->ip[0] = s->sin_addr.s_addr;
}

/*
================
NET_IPSocket
================
*/
int NET_IPSocket( const char *net_interface, int port, netadr_t *bound_to ) {
	int newsocket;
	sockaddr_in address;

	if ( net_interface ) {
		NET_StringToSockaddr( net_interface, &address, true );
	} else {
		memset( &address, 0, sizeof( address ) );
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}

	if ( port == PORT_ANY ) {
		address.sin_port = 0;
	} else {
		address.sin_port = htons( (short)port );
	}

	if ( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 ) {
		return -1;
	}

	if ( bind( newsocket, (sockaddr *)&address, sizeof( address ) ) == -1 ) {
		close( newsocket );
		return -1;
	}

	if ( bound_to ) {
		socklen_t len = sizeof( address );
		getsockname( newsocket, (sockaddr *)&address, &len );
		Net_SockadrToNetadr( &address, bound_to );
	}

	int i = 1;
	setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof( i ) );

	return newsocket;
}

/*
================
Net_WaitForData
================
*/
bool Net_WaitForData( int netSocket, int timeout ) {
	fd_set fdset;
	struct timeval to;

	FD_ZERO( &fdset );
	FD_SET( netSocket, &fdset );

	to.tv_sec = timeout / 1000;
	to.tv_usec = ( timeout % 1000 ) * 1000;

	int ret = select( netSocket + 1, &fdset, NULL, NULL, timeout == 0 ? NULL : &to );
	return ret > 0;
}

/*
================
Net_GetUDPPacket
================
*/
bool Net_GetUDPPacket( int netSocket, netadr_t &net_from, char *data, int &size, int maxSize ) {
	int ret;
	socklen_t fromlen;
	sockaddr_in from;

	if ( netSocket == -1 ) {
		return false;
	}

	fromlen = sizeof( from );
	ret = recvfrom( netSocket, data, maxSize, 0, (sockaddr *)&from, &fromlen );
	if ( ret == -1 ) {
		return false;
	}

	Net_SockadrToNetadr( &from, &net_from );
	size = ret;
	return true;
}

/*
================
Net_SendUDPPacket
================
*/
void Net_SendUDPPacket( int netSocket, int length, const void *data, const netadr_t to ) {
	sockaddr_in addr;

	if ( netSocket == -1 ) {
		return;
	}

	Net_NetadrToSockadr( &to, &addr );
	sendto( netSocket, (const char *)data, length, 0, (sockaddr *)&addr, sizeof( addr ) );
}

/*
================
Sys_StringToSockaddr
================
*/
bool NET_StringToSockaddr( const char *s, sockaddr_in *sadr, bool doDNSResolve ) {
	memset( sadr, 0, sizeof( *sadr ) );
	sadr->sin_family = AF_INET;

	char buf[256];
	hfStr::Copynz( buf, s, sizeof( buf ) );

	// check for a numeric port split from the addr
	char * port = strchr( buf, ':' );
	if ( port ) {
		*port = '\0';
		port++;
	}

	// numeric IPv4
	int a, b, c, d;
	if ( sscanf( buf, "%d.%d.%d.%d", &a, &b, &c, &d ) == 4
		&& a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255 ) {
		((unsigned char *)&sadr->sin_addr)[0] = (unsigned char)a;
		((unsigned char *)&sadr->sin_addr)[1] = (unsigned char)b;
		((unsigned char *)&sadr->sin_addr)[2] = (unsigned char)c;
		((unsigned char *)&sadr->sin_addr)[3] = (unsigned char)d;
	} else {
		if ( !doDNSResolve ) {
			return false;
		}
		sockaddr_in tmpsa;
		struct hostent * h = gethostbyname( buf );
		if ( h == NULL || h->h_addrtype != AF_INET ) {
			return false;
		}
		memcpy( &sadr->sin_addr, h->h_addr_list[0], h->h_length );
	}

	if ( port ) {
		sadr->sin_port = htons( (short)atoi( port ) );
	}

	return true;
}

/*
================
Sys_StringToNetAdr
================
*/
bool Sys_StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve ) {
	sockaddr_in sadr;
	if ( !NET_StringToSockaddr( s, &sadr, doDNSResolve ) ) {
		return false;
	}
	Net_SockadrToNetadr( &sadr, a );
	return true;
}

/*
================
Sys_NetAdrToString
================
*/
const char *Sys_NetAdrToString( const netadr_t a ) {
	static char s[64];
	if ( a.type == NA_LOOPBACK ) {
		return "loopback";
	}
	hfStr::snPrintf( s, sizeof( s ), "%d.%d.%d.%d:%d", a.ip[0], a.ip[1], a.ip[2], a.ip[3], a.port );
	return s;
}

/*
================
Sys_IsLANAddress
================
*/
bool Sys_IsLANAddress( const netadr_t adr ) {
	if ( adr.type == NA_LOOPBACK ) {
		return true;
	}
	if ( adr.type != NA_IP ) {
		return false;
	}
	// 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
	if ( adr.ip[0] == 10 ) {
		return true;
	}
	if ( adr.ip[0] == 172 && ( adr.ip[1] >= 16 && adr.ip[1] <= 31 ) ) {
		return true;
	}
	if ( adr.ip[0] == 192 && adr.ip[1] == 168 ) {
		return true;
	}
	return false;
}

/*
================
Sys_CompareNetAdrBase
================
*/
bool Sys_CompareNetAdrBase( const netadr_t a, const netadr_t b ) {
	if ( a.type != b.type ) {
		return false;
	}
	if ( a.type == NA_LOOPBACK ) {
		return true;
	}
	if ( a.type == NA_IP && memcmp( a.ip, b.ip, 4 ) == 0 ) {
		return true;
	}
	return false;
}

/*
================
Sys_GetLocalIPCount
================
*/
int Sys_GetLocalIPCount() {
	struct ifaddrs * ifaddr, * ifa;
	int count = 0;
	if ( getifaddrs( &ifaddr ) == -1 ) {
		return 0;
	}
	for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next ) {
		if ( ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET ) {
			continue;
		}
		if ( ( ifa->ifa_flags & IFF_LOOPBACK ) == 0 ) {
			count++;
		}
	}
	freeifaddrs( ifaddr );
	return count;
}

/*
================
Sys_GetLocalIP
================
*/
const char * Sys_GetLocalIP( int i ) {
	static char ip[16];
	struct ifaddrs * ifaddr, * ifa;
	int index = 0;
	if ( getifaddrs( &ifaddr ) == -1 ) {
		return "127.0.0.1";
	}
	for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next ) {
		if ( ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET ) {
			continue;
		}
		if ( ifa->ifa_flags & IFF_LOOPBACK ) {
			continue;
		}
		if ( index == i ) {
			struct sockaddr_in * sin = (struct sockaddr_in *)ifa->ifa_addr;
			inet_ntop( AF_INET, &sin->sin_addr, ip, sizeof( ip ) );
			freeifaddrs( ifaddr );
			return ip;
		}
		index++;
	}
	freeifaddrs( ifaddr );
	return "127.0.0.1";
}

/*
================
hfUDP
================
*/
hfUDP::hfUDP() {
	netSocket = 0;
	memset( &bound_to, 0, sizeof( bound_to ) );
	packetsRead = 0;
	bytesRead = 0;
	packetsWritten = 0;
	bytesWritten = 0;
}

hfUDP::~hfUDP() {
	Close();
}

bool hfUDP::InitForPort( int portNumber ) {
	Close();
	netSocket = NET_IPSocket( net_ip.GetString(), portNumber, &bound_to );
	if ( netSocket == -1 ) {
		netSocket = 0;
		return false;
	}
	return true;
}

void hfUDP::Close() {
	if ( netSocket > 0 ) {
		close( netSocket );
	}
	netSocket = 0;
}

bool hfUDP::GetPacket( netadr_t &from, void *data, int &size, int maxSize ) {
	if ( !Net_GetUDPPacket( netSocket, from, (char *)data, size, maxSize ) ) {
		return false;
	}
	packetsRead++;
	bytesRead += size;
	return true;
}

bool hfUDP::GetPacketBlocking( netadr_t &from, void *data, int &size, int maxSize, int timeout ) {
	if ( !Net_WaitForData( netSocket, timeout ) ) {
		return false;
	}
	return GetPacket( from, data, size, maxSize );
}

void hfUDP::SendPacket( const netadr_t to, const void *data, int size ) {
	Net_SendUDPPacket( netSocket, size, data, to );
	packetsWritten++;
	bytesWritten += size;
}

