//
// GreenSQL DB Proxy class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_GREENSQL_HPP
#define GREEN_SQL_GREENSQL_HPP

#include "connection.hpp"
#include "log.hpp"
#include "config.hpp"
#ifdef WIN32
#include <winsock2.h>
#endif

#include <event.h>
#include <string>
#include <vector>
class GreenSQL
{
    

public:

    GreenSQL();

    virtual ~GreenSQL(void);
    
    //socket
    int server_socket(std::string & ip, int port);
    int client_socket(std::string & server, int port);
    int socket_accept(int serverfd);
    int static socket_close(int sfd);
    bool socket_read(int fd, char * data, int & size);
    bool socket_write(int fd, const char* data, int & size);
    int new_socket();
    
    //proxy
    bool ProxyInit(int proxyId, std::string & proxyIp, int proxyPort,
                   std::string & backendIp, int backendPort);
    bool ProxyReInit(int proxyId, std::string & proxyIp, 
		    int proxyPort, std::string & bIp, int bPort);
    bool ServerInitialized();	
    bool PrepareNewConn(int, int &, int &);
    virtual void Server_cb(int fd, short which, void * arg, 
    		Connection *, int, int);
    void Proxy_cb(int fd, short which, void * arg);
    void Proxy_write_cb(int fd, short which, void * arg);
    void ProxyValidateClientRequest(Connection*);
    void Backend_cb(int fd, short which, void * arg);
    void Backend_write_cb(int fd, short which, void * arg);
    void ProxyValidateServerResponse(Connection*);
    
    void Close();
    void CloseConnection(Connection * conn);

    std::vector<Connection*> v_conn;
    struct event serverEvent;

    int         iProxyId;
    std::string sBackendName;
    bool bBackendName;
    std::string sBackendIP;
    int         iBackendPort;
    std::string sProxyIP;
    int         iProxyPort;
    bool bRunning;
 
};

#endif
