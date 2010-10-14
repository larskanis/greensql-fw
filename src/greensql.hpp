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
#include <list>
typedef void (*pt2Func)(int, short, void *);
void Proxy_cb(int fd, short which, void * arg);
bool Proxy_write_cb(int fd, Connection * conn);
bool ProxyValidateClientRequest(Connection*);
bool ProxyValidateServerResponse(Connection*);
void Backend_cb(int fd, short which, void * arg);
bool Backend_write_cb(int fd, Connection * conn);
bool socket_read(int fd, char * data, int & size);
bool socket_write(int fd, const char* data, int & size);
void CloseConnection(Connection * conn);
void clear_init_event(Connection * conn, int fd, short flags, pt2Func func, void * params,bool proxy = true);

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
    //bool socket_read(int fd, char * data, int & size);
    //bool socket_write(int fd, const char* data, int & size);
    int new_socket();
    
    //proxy
    bool ProxyInit(int proxyId, std::string & proxyIp, int proxyPort,
                   std::string & backendName, std::string & backendIp, int backendPort,
		   std::string & dbType);
    bool ProxyReInit(int proxyId, std::string & proxyIp, int proxyPort, 
                   std::string & backendName, std::string & bIp, int bPort,
		   std::string & dbType);
    bool ServerInitialized();
    bool HasActiveConnections();
    bool PrepareNewConn(int, int &, int &);
    virtual void Server_cb(int fd, short which, void * arg, 
    		Connection *, int, int);
    //void Proxy_cb(int fd, short which, void * arg);
    //void Proxy_write_cb(int fd, short which, void * arg);
    //void ProxyValidateClientRequest(Connection*);
    //void Backend_cb(int fd, short which, void * arg);
    //void Backend_write_cb(int fd, short which, void * arg);
    //void ProxyValidateServerResponse(Connection*);
    
    void Close();
    void CloseServer();
    std::list<Connection*> v_conn;
    struct event serverEvent;

    int         iProxyId;
    std::string sBackendName;
    bool bBackendName;
    std::string sBackendIP;
    int         iBackendPort;
    std::string sProxyIP;
    int         iProxyPort;
    std::string sDBType;
    DBProxyType     DBType;

    bool bRunning;
 
};

#endif
