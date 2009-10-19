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

#include <string.h>

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

static const int min_buf_size = 10240;

GreenSQL::GreenSQL()
{
    memset(&serverEvent, 0, sizeof(struct event));
    //v_conn.reserve(10);
}

GreenSQL::~GreenSQL(void)
{
    //logevent(DEBUG, "i am in GreenSQL destructor\n");
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
     
    event_set( &conn->backend_event, cfd, EV_READ | EV_PERSIST, 
               wrap_Backend, (void *)conn);
    event_add( &conn->backend_event,0);
    logevent(NET_DEBUG, "size of the connection queue: %d\n", v_conn.size());

    v_conn.push_front(conn);

    conn->connections = &v_conn;
    conn->location = v_conn.begin();

    // get db user ip address
    struct sockaddr_in ss;
    unsigned int len = sizeof(struct sockaddr_in);

    getpeername(sfd,(struct sockaddr*)&ss,&len);
    conn->db_user_ip = inet_ntoa(ss.sin_addr);
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
        logevent(NET_DEBUG, "Failed to bind server socket on %s:%d\n", ip.c_str(), port);
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
            logevent(NET_DEBUG, "[%d] Failed to connect to backend server, too many open sockets\n", sfd);
        } else {
            logevent(NET_DEBUG, "[%d] Failed to connect to backend server\n", sfd);
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
            return sfd;
        } else if (errno == EMFILE) {
            logevent(NET_DEBUG, "[%d] Failed to connect to backend server, too many open sockets\n", sfd);
        } else {
            logevent(NET_DEBUG, "[%d] Failed to connect to backend server\n", sfd);
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
            logevent(NET_DEBUG, "[%d] Failed to accept client socket, too many open sockets\n", serverfd);
        } else {
            logevent(NET_DEBUG, "[%d] Failed to accept client socket\n", serverfd);
        }
    #else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
            return sfd;
        } else if (errno == EMFILE) {
            logevent(NET_DEBUG, "[%d] Failed to accept client socket, too many open sockets\n", serverfd);
        } else {
            logevent(NET_DEBUG, "[%d] Failed to accept client socket\n", serverfd);
        }
    #endif
        socket_close(sfd);
        return -1;
    }
    
    #ifdef WIN32
    unsigned long nonblock  = 1;
    if (ioctlsocket(sfd, FIONBIO, &nonblock) == SOCKET_ERROR)
    {
        logevent(NET_DEBUG, "[%d] Failed to swith socket to non-blocking mode\n", sfd);
        socket_close(sfd);
        return -1;
    }
    #else
    int flags;
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
         fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        logevent(NET_DEBUG, "[%d] Failed to swith socket to non-blocking mode\n", sfd); 
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

bool socket_read(int fd, char * data, int & size)
{
    if ((size = recv(fd, data, size, 0)) < 0)
    {
        size = 0;
#ifdef WIN32
        int err = WSAGetLastError();
        logevent(NET_DEBUG, "[%d] Socket read error %d\n", fd, err);
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            return true;
        }
#else
        logevent(NET_DEBUG, "[%d] Socket read error %d\n", fd, errno);
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
           return true;
        }
#endif
        return false;
    }
    if (size == 0)
    {
       logevent(NET_DEBUG, "[%d] Socket read error %d\n", fd, errno);
       return false;
    }
                   
    return true; 
}

bool socket_write(int fd, const char* data, int & size)
{
    logevent(NET_DEBUG, "Socket_write\n");
    if ((size = send(fd, data, size, 0))  <= 0)
    {
#ifdef WIN32
        int err = WSAGetLastError();
        logevent(NET_DEBUG, "[%d] Socket write error %d\n", fd, err);
        if (err == WSAEINTR ||err == WSAEWOULDBLOCK|| err == WSAEINPROGRESS ) {
            size = 0;
            return true;
        }
#else
        logevent(NET_DEBUG, "[%d] Socket write error %d\n", fd, errno);
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
        logevent(NET_DEBUG, "[%d] Failed to swith socket to non-blocking mode\n", sock);
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
        logevent(NET_DEBUG, "[%d] Failed to swith socket to non-blocking mode\n", sfd);
        socket_close(sfd);
        return -1;
    }
#endif
    return sfd;
}


bool GreenSQL::ProxyInit(int proxyId, std::string & proxyIp, int proxyPort,
        std::string & backendIp, int backendPort, std::string & dbType)
{
   
    sProxyIP = proxyIp;
    iProxyPort = proxyPort;
    sBackendName = backendIp;
    iBackendPort = backendPort;
    iProxyId = proxyId;
    sDBType = dbType;
    if (strcasecmp(dbType.c_str(), "mysql") == 0)
    {
        DBType = DBTypeMySQL;
    } else if (strcasecmp(dbType.c_str(), "pgsql") == 0)
    {
        DBType = DBTypePGSQL;
    } else if (strcasecmp(dbType.c_str(), "mssql") == 0)
    {
        DBType = DBTypeMSSQL;
    } else
    {
        DBType = DBTypeMySQL;
    }
    int sfd = server_socket(sProxyIP, iProxyPort);
    if (sfd == -1)
        return false;

    event_set(&serverEvent, sfd, EV_READ | EV_WRITE | EV_PERSIST,
              wrap_Server, (void *)iProxyId);

    event_add(&serverEvent, 0);    
    return true;
}

bool GreenSQL::ProxyReInit(int proxyId, std::string & proxyIp, int proxyPort,
        std::string & backendIp, int backendPort,
        std::string & dbType)
{
    if (ServerInitialized())
    {
        event_del(&serverEvent);
        socket_close(serverEvent.ev_fd);
        serverEvent.ev_fd = 0;
    }
    return ProxyInit(proxyId, proxyIp, proxyPort, backendIp, backendPort, dbType);
}

// this function return true is server socket is established
bool GreenSQL::ServerInitialized()
{
    if (serverEvent.ev_fd != 0 && serverEvent.ev_fd != -1 && 
        serverEvent.ev_flags & EVLIST_INIT)
        return true;
    return false;
}

void Proxy_cb(int fd, short which, void * arg)
{
    Connection * conn = (Connection *) arg;
    char data[min_buf_size];
    int len = sizeof(data) - 1;

    if (which & EV_WRITE)
    {
      // trying to send data
      if (Proxy_write_cb(fd, conn) == false)
      {
        // failed to send data. close this socket.
        CloseConnection(conn);
        return;
      }
    }

    if (!(which & EV_READ))
        return;

    if (socket_read(fd, data, len) == false)
    {
        logevent(NET_DEBUG, "[%d] Failed to read socket, closing socket\n", fd);
        CloseConnection(conn);
        return;
    }
    
    if ( len > 0 )
    {
        data[len] = '\0';
        //printf("received data\n");
        conn->request_in.append(data,len);
    
        //perform validation of the request
        if (ProxyValidateClientRequest(conn) == false)
        {
            CloseConnection(conn);
        }
    }
}

bool Proxy_write_cb(int fd, Connection * conn)
{
    int len = conn->response_out.size();

    if (len == 0)
    {
        if (conn->proxy_event.ev_flags & EV_WRITE)
        {
            //we can clear the WRITE event flag
            event_del( &conn->proxy_event);
            event_set( &conn->proxy_event, fd, EV_READ | EV_PERSIST,
                       wrap_Proxy, (void *)conn);
            event_add( &conn->proxy_event, 0);
        }
        return true;
    }

    const unsigned char * data = conn->response_out.raw();

    if (socket_write(fd, (const char * )data, len) == true)
    {
        logevent(NET_DEBUG, "Send data to client\n");
        conn->response_out.chop_back(len);
    } else {
        logevent(NET_DEBUG, "[%d] Failed to send, closing socket\n", fd);
        // no need to close socket object here
        // it will be done in the caller function
        return false;
    }
    if (conn->response_out.size() == 0 &&
        conn->proxy_event.ev_flags & EV_WRITE)
    {
        //we can clear the WRITE event flag
        event_del( &conn->proxy_event);
        event_set( &conn->proxy_event, fd, EV_READ | EV_PERSIST, 
                   wrap_Proxy, (void *)conn);
        event_add( &conn->proxy_event, 0);
    } else if (conn->response_out.size() > 0 &&
        !(conn->proxy_event.ev_flags & EV_WRITE))
    {
        // we need to enable WRITE event flag
        event_del( &conn->proxy_event);
        event_set( &conn->proxy_event, fd, EV_READ | EV_WRITE | EV_PERSIST,
                   wrap_Proxy, (void *)conn);
        event_add( &conn->proxy_event, 0);
    }
    return true;
}

bool ProxyValidateClientRequest(Connection * conn)
{
    std::string request = "";
    bool hasResponse = false;
    request.reserve(min_buf_size);
    int len = 0;

    // mysql_validate(request);
    if (conn->parseRequest(request, hasResponse) == false)
    {
        logevent(NET_DEBUG, "Failed to parse request, closing socket.\n");
        return false;
    }

    len = (int)request.size();

    if (hasResponse == true)
    {
        // we can push response to response_in or response_out queues
        if (conn->response_in.size() != 0)
        {
            if (ProxyValidateServerResponse(conn) == false)
            {
                return false;
            }
        } else if (conn->response_out.size() != 0)
        {
            Proxy_write_cb( conn->proxy_event.ev_fd, conn);
        }
    }
    if (len <= 0)
        return true;

    //push request
    conn->request_out.append(request.c_str(), (int)request.size());

    return Backend_write_cb(conn->backend_event.ev_fd, conn);
}

void Backend_cb(int fd, short which, void * arg)
{

    //we are real server client.
    Connection * conn = (Connection *) arg;
    char data[min_buf_size];
    int len = sizeof(data)-1;

    // check if we can write to this socket
    if ( which & EV_WRITE )
    {
        if ( Backend_write_cb(fd, conn) == false )
        {
            // failed to write, close this socket
            CloseConnection(conn);
            return;
        }
    }

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

        if (ProxyValidateServerResponse(conn) == false)
        {
            CloseConnection(conn);
        }
    }

    return;
}

bool Backend_write_cb(int fd, Connection * conn)
{
    //we are real server client.
    int len = conn->request_out.size();

    if (len == 0)
    {
        if (conn->backend_event.ev_flags & EV_WRITE)
        {
            //we can clear the WRITE event flag
            event_del( &conn->backend_event);
            event_set( &conn->backend_event, fd, EV_READ | EV_PERSIST, 
                       wrap_Backend, (void *)conn);
            event_add( &conn->backend_event, 0);
        }
        return true;   
    }
    const unsigned char * data = conn->request_out.raw();

    if (socket_write(fd, (const char *)data, len) == true)
    {
        logevent(NET_DEBUG, "sending data to backend server\n");
        conn->request_out.chop_back(len);
    } else {
        logevent(NET_DEBUG, "[%d] Failed to send data to client, closing socket\n", fd);
        // no need to close connection here
        return false;
    }
    if (conn->request_out.size() == 0 && 
        conn->backend_event.ev_flags & EV_WRITE)
    {
        //we can clear the WRITE event flag
        event_del( &conn->backend_event);
        event_set( &conn->backend_event, fd, EV_READ | EV_PERSIST, 
                   wrap_Backend, (void *)conn);
        event_add( &conn->backend_event, 0);
    } else if (conn->request_out.size() > 0 && 
        !(conn->backend_event.ev_flags & EV_WRITE))
    {
        // we need to enable WRITE event flag
        event_del( &conn->backend_event);
        event_set( &conn->backend_event, fd, EV_READ | EV_WRITE | EV_PERSIST, 
                   wrap_Backend, (void *)conn);
        event_add( &conn->backend_event, 0);
    }
    return true;
}

bool ProxyValidateServerResponse( Connection * conn )
{
    std::string response;
    response.reserve(min_buf_size);

    conn->parseResponse(response);
    
    //push response
    if (response.size() == 0)
    {
        return true;
    }
    conn->response_out.append(response.c_str(), (int)response.size());
    // if an error occurred while sending data, this socket will be closed.
    return Proxy_write_cb( conn->proxy_event.ev_fd, conn);
}

void GreenSQL::Close()
{
    //check if we have initialized server socket
    
    Connection * conn;
    while ( v_conn.size() != 0)
    {
        conn = v_conn.front();
        // we remove pointer to the conn object inside the close() function
        conn->close();
        delete conn;
    }
    v_conn.clear();

    if (ServerInitialized())
    {
        socket_close(serverEvent.ev_fd);
        event_del(&serverEvent);
        serverEvent.ev_fd = 0;
    }

    //logevent(NET_DEBUG, "Closing proxy object\n");
}

void CloseConnection(Connection * conn)
{
    conn->close();
    delete conn;
}

