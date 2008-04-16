//
// GreenSQL DB Proxy class implementation.
// This code has OS dependent code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "greensql.hpp"


#include "config.hpp"
#include "connection.hpp"
#include "proxymap.hpp"
#include <vector>
#include <iostream>


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif


GreenSQL::GreenSQL()
{
    //v_conn.reserve(10);
}

GreenSQL::~GreenSQL(void)
{
    logevent(DEBUG, "i am in GreenSQL destructor\n");
}

bool GreenSQL::PrepareNewConn(int fd, int & sfd, int & cfd)
{
    //logevent(NET_DEBUG, "server socket fired, fd=%d\n",fd);
    sfd = socket_accept(fd);
    if (sfd == -1)
        return false;

    cfd = client_socket(sBackendName, iBackendPort);
    if (cfd == -1)
    {
        socket_close(sfd);
	logevent(NET_DEBUG, "Failed to connect to backend db server (%s:%d)\n", sBackendName.c_str(), iBackendPort);
	return false;
    }
    
    //logevent(NET_DEBUG, "client (to mysql backend) connection established\n");
    return true;
}

void GreenSQL::Server_cb(int fd, short which, void * arg, 
		Connection * conn, int sfd, int cfd)
{
    logevent(NET_DEBUG, "GreenSQL Server_cb(), sfd=%d, cfd=%d\n", sfd, cfd);
        
    event_set( &conn->proxy_event, sfd, EV_READ | EV_PERSIST, 
               wrap_Proxy, (void *)conn);
    event_add( &conn->proxy_event,0);
     
    event_set( &conn->client_event, cfd, EV_READ | EV_PERSIST, 
               wrap_Backend, (void *)conn);
    event_add( &conn->client_event,0);
    logevent(NET_DEBUG, "size of the connection queue: %d\n", v_conn.size());
	    
    v_conn.push_back(conn);
}


int GreenSQL::server_socket(std::string & ip, int port)
{
    int sfd;
    struct linger ling = {0, 0};
    struct sockaddr_in addr;
    int flags =1;

    if ((sfd = new_socket()) == -1) {
        return -1;
    }

    #ifdef WIN32
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));
    setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags));
    #else
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
    setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
    #endif
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    //addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        logevent(NET_DEBUG, "Failed to bind server socket\n");
        socket_close(sfd);
        return -1;
    }
    
    if (listen(sfd, 1024) == -1) {
        logevent(NET_DEBUG, "Failed to switch socket to listener mode\n");
        socket_close(sfd);
        return -1;
    }

    return sfd;
}

int GreenSQL::client_socket(std::string & server, int port)
{
    int sfd;
    struct sockaddr_in addr;
    int flags =1;
    
    if ((sfd = new_socket()) == -1) {
        return -1;
    }

    #ifdef WIN32
    setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flags, sizeof(flags));
    #else
    setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    #endif

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(server.c_str());
      
    if (connect(sfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            return sfd;
        } else if (err == WSAEMFILE) {
            perror("Too many open connections\n");
        } else {
            perror("connect()");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
            return sfd;
        } else if (errno == EMFILE) {
            perror("Too many open connections\n");
        } else {
            perror("connect()");
        }
#endif
        socket_close(sfd);
	    return -1;
    }
    return sfd;
}

int GreenSQL::socket_accept(int serverfd)
{
    socklen_t addrlen;
    struct sockaddr addr;
    int sfd;
    
    memset(&addr, 0, sizeof(addr));
    addrlen = sizeof(addr);

    if ((sfd = (int)accept(serverfd, &addr, &addrlen)) == -1) {
    #ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            return sfd;
        } else if (err == WSAEMFILE) {
            perror("Too many open connections\n");
        } else {
            perror("connect()");
        }
    #else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
            return sfd;
        } else if (errno == EMFILE) {
            perror("Too many open connections\n");
        } else {
            perror("connect()");
        }
    #endif
	socket_close(sfd);
        return -1;
    }
    
    #ifdef WIN32
    unsigned long nonblock  = 1;
    if (ioctlsocket(sfd, FIONBIO, &nonblock) == SOCKET_ERROR)
    {
        perror("setting O_NONBLOCK");
        socket_close(sfd);
	return -1;
    }
    #else
    int flags;
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
         fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        socket_close(sfd);
	return -1;
    }
    #endif
    return sfd;
}

int GreenSQL::socket_close(int sfd)
{
#ifdef WIN32
    closesocket(sfd);
#else
    close(sfd);
#endif
    return 0;
}

bool GreenSQL::socket_read(int fd, char * data, int & size)
{
    if ((size = recv(fd, data, size, 0)) < 0)
    {
        size = 0;
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            return true;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
           return true;
        }
#endif
        return false;
    }
    if (size == 0)
    {
       return false;
    }
                   
    return true; 
}

bool GreenSQL::socket_write(int fd, const char* data, int & size)
{
    logevent(NET_DEBUG, "Socket_write\n");
    if ((size = send(fd, data, size, 0))  <= 0)
    {
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            size = 0;
            return true;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
           size = 0;
           return true;
        }
#endif
        return false;
    }

    return true;
  
}

int GreenSQL::new_socket() {
    int sfd;

#ifdef WIN32
    unsigned long nonblock  = 1;
    SOCKET sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        logevent(NET_DEBUG, "Failed to create socket\n");
	return -1;
    }

    if (ioctlsocket(sock, FIONBIO, &nonblock) == SOCKET_ERROR)
    {
        perror("setting O_NONBLOCK");
        socket_close((int)sock);
	return -1;
    }
    sfd = (int) sock;
#else

    if ((sfd = (int)socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        logevent(NET_DEBUG, "Failed to create socket\n");
	return -1;
    }

    int flags;
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 || 
         fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("setting O_NONBLOCK");
        socket_close(sfd);
	return -1;
    }
#endif
    return sfd;
}


bool GreenSQL::ProxyInit(int proxyId, std::string & proxyIp, int proxyPort,
		std::string & backendIp, int backendPort)
{
   
    sProxyIP = proxyIp;
    iProxyPort = proxyPort;
    sBackendName = backendIp;
    iBackendPort = backendPort;
    iProxyId = proxyId;

    int sfd = server_socket(sProxyIP, iProxyPort);
    if (sfd == -1)
        return false;

    event_set(&serverEvent, sfd, EV_READ | EV_WRITE | EV_PERSIST,
              wrap_Server, (void *)iProxyId);

    event_add(&serverEvent, 0);    
    return true;
}

bool GreenSQL::ProxyReInit(int proxyId, std::string & proxyIp, int proxyPort,
		                std::string & backendIp, int backendPort)
{
    if (ServerInitialized())
    {
        event_del(&serverEvent);
	
	socket_close(serverEvent.ev_fd);
	
        serverEvent.ev_fd = 0;
    }
    return ProxyInit(proxyId, proxyIp, proxyPort, backendIp, backendPort);
}

// this function return true is server socket is established
bool GreenSQL::ServerInitialized()
{
    if (serverEvent.ev_fd != 0 && serverEvent.ev_fd != -1 && 
		serverEvent.ev_flags & EVLIST_INIT)
        return true;
    return false;
}

void GreenSQL::Proxy_cb(int fd, short which, void * arg)
{
    //remote user sends us a command
    try
    {
        Connection * conn = (Connection *) arg;
        char data[1024];
        int len = sizeof(data) - 1;

        if (which & EV_WRITE)
            Proxy_write_cb(fd, which, arg);

        if (!(which & EV_READ))
            return;

        if (socket_read(fd, data, len) == false)
        {
            logevent(NET_DEBUG, "Failed to read socket, closing socket\n");
            CloseConnection(conn);
            return;
        }
    
        if ( len > 0 )
        {
            data[len] = '\0';
            //printf("received data\n");
            conn->request_in.append(data,len);
    
            //perform validation of the request
            ProxyValidateClientRequest(conn);
        }
    }
    catch(char * str)
    {
        std::cout << "Proxy_cb exception:" << str << std::endl;
        return;
    }
}

void GreenSQL::Proxy_write_cb(int fd, short which, void * arg)
{
    try
    {
        Connection * conn = (Connection *) arg;
        int len = conn->response_out.size();
        const unsigned char * data = conn->response_out.raw();

        if (socket_write(fd, (const char * )data, len) == true)
        {
            logevent(NET_DEBUG, "Proxy data send\n");
            conn->response_out.chop_back(len);
        } else {
	    logevent(NET_DEBUG, "Failed to send, closing socket\n");
            CloseConnection(conn);
            return;
        }
        if (conn->response_out.size() == 0)
        {
            //we can clear the WRITE event flag
            event_del( &conn->proxy_event);
            event_set( &conn->proxy_event, fd, EV_READ | EV_PERSIST, 
		wrap_Proxy, (void *)conn);
            event_add( &conn->proxy_event, 0);
        } 
    }
    catch(char * str)
    {
        std::cout << "Proxy_write_cb exception:" << str << std::endl;
        return;
    }
}

void GreenSQL::ProxyValidateClientRequest(Connection * conn)
{
    std::string request = "";
    bool hasResponse = false;
    request.reserve(1024);

    // mysql_validate(request);
    try
    {
        if (conn->parseRequest(request, hasResponse) == false)
	    {
            logevent(NET_DEBUG, "Failed to parse request, closing socket.\n");
            CloseConnection(conn);
	    }

        int len = (int)request.size();

        if (hasResponse == true)
        {
            ProxyValidateServerResponse( conn ); 
        }
        else if (len > 0)
        {
            //try to write without polling
            if (socket_write(conn->client_event.ev_fd, 
			request.c_str(), len) == true)
            {
                if (request.size() == (unsigned int)len)
              	    return;
                request.erase(0,len);
            } else {
                logevent(NET_DEBUG, "Failed to write to backend server\n");
                CloseConnection(conn);
            }

            //push request
            conn->request_out.append(request.c_str(), (int)request.size());

            //now we need to check if client event is set to WRITE
            if (conn->client_event.ev_flags != 
	        (EV_READ | EV_WRITE | EV_PERSIST) )
            {
                int fd = conn->client_event.ev_fd;
                event_del( &conn->client_event);
                event_set( &conn->client_event, fd, 
                          EV_READ | EV_WRITE | EV_PERSIST, 
                wrap_Backend, (void *)conn);
                event_add( &conn->client_event,0);
            }
	    }
    }
    catch(char * str)
    {
        std::cout << "ProxyValidateClientRequest exception:" << str << std::endl;
        return;
    }
}

void GreenSQL::Backend_cb(int fd, short which, void * arg)
{
    try
    {
        //we are real server client.
        Connection * conn = (Connection *) arg;
        char data[1024];
        int len = sizeof(data)-1;

        if ( which & EV_WRITE )
            Backend_write_cb(fd, which, arg);

        if (!(which & EV_READ))
            return;
    
        if (socket_read(fd, data, len) == false)
        {
            CloseConnection(conn);
            return;
        }
        if ( len > 0 )
        {
            data[len] = '\0';
            //printf("received data\n");
            conn->response_in.append(data,len);

            ProxyValidateServerResponse( conn );
        }
        return;
    }
    catch(char * str)
    {
        std::cout << "Backend_cb exception:" << str << std::endl;
        return;
    }
}

void GreenSQL::Backend_write_cb(int fd, short which, void * arg)
{
    try
    {
        //we are real server client.
        Connection * conn = (Connection *) arg;
        int len = conn->request_out.size();
        const unsigned char * data = conn->request_out.raw();

        if (socket_write(fd, (const char *)data, len) == true)
        {
	    logevent(NET_DEBUG, "sending data to client\n");
            //printf("client data send\n");
            conn->request_out.chop_back(len);
        } else {
	    logevent(NET_DEBUG, "Failed to send data to client, closing socket\n");
            CloseConnection(conn);
            return;
        }
        if (conn->request_out.size() == 0)
        {
            //we can clear the WRITE event flag
            event_del( &conn->client_event);
            event_set( &conn->client_event, fd, EV_READ | EV_PERSIST, 
			    wrap_Backend, (void *)conn);
            event_add( &conn->client_event, 0);
        }
    }
    catch(char * str)
    {
        std::cout << "Backend_write_cb exception:" << str << std::endl;
        return;
    }
}

void GreenSQL::ProxyValidateServerResponse( Connection * conn )
{
    std::string response;
    response.reserve(1024);

    try
    {
        conn->parseResponse(response);
    
        //try to write without polling
        int len = (int)response.size();
	    if (len == 0)
	    {
            // need to read more data to decide
	        return;
	    }
        if (socket_write(conn->proxy_event.ev_fd, response.c_str(), len) == true)
        {
            if (response.size() == (unsigned int)len)
            	return;
            response.erase(0,len);
        } else {
            CloseConnection(conn);
            return;
        }
    
        //push respose
        conn->response_out.append(response.c_str(), (int)response.size());

        //now we need to check if proxy event is set to WRITE
        if (conn->proxy_event.ev_flags != (EV_READ | EV_WRITE | EV_PERSIST) )
        {
            int fd = conn->proxy_event.ev_fd;
            event_del( &conn->proxy_event);
            event_set( &conn->proxy_event, fd, EV_READ | EV_WRITE | EV_PERSIST, wrap_Proxy, (void *)conn);
            event_add( &conn->proxy_event, 0);
        }
    }
    catch(char * str)
    {
        std::cout << "ProxyValidateServerResponse exception:" << str << std::endl;
        return;
    }
    
}

void GreenSQL::Close()
{
    //check if we have initialized server socket
    
    Connection * conn;
    size_t i = v_conn.size();
    while ( i != 0)
    {
        conn = v_conn[i-1];
        conn->close();
        v_conn.pop_back();
	delete conn;
        i--;
    }
    v_conn.clear();

    if (ServerInitialized())
    {
        socket_close(serverEvent.ev_fd);
        event_del(&serverEvent);
        serverEvent.ev_fd = 0;
    }

    logevent(NET_DEBUG, "Closing proxy object\n");
}

void GreenSQL::CloseConnection(Connection * conn)
{
    std::vector<Connection*>::iterator itr;

    //remove frpm v_conn

    itr = v_conn.begin();
    while (itr != v_conn.end())
    {
        if (*itr == conn)
        {
            v_conn.erase(itr);
            break;
        }
        itr++;
    }
    conn->close();
    delete conn;
}

