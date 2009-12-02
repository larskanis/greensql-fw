//
// GreenSQL functions used to manage a number of proxy objects.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "greensql.hpp"
#include "proxymap.hpp"
#include "mysql/mysql_con.hpp"
#include "pgsql/pgsql_con.hpp"

#include <stdio.h>
#include<map>

static const char * const q_proxy = "SELECT proxyid, frontend_ip, frontend_port, backend_server, "
            "backend_ip, backend_port, dbtype FROM proxy WHERE status != 3";
        
static std::map<int, GreenSQL * > proxies;

void wrap_Server(int fd, short which, void * arg)
{
    long proxy_id;
    
    proxy_id = (long) arg;
    logevent(NET_DEBUG, "[%d]wrap_Server\n", proxy_id);
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    
    GreenSQL * cls = proxies[proxy_id];
    Connection * conn = NULL;
    int sfd;
    int cfd;
 
    if(cls == NULL)
      return;
    if (conf->bRunning == false)
    {
      cls->Close();
    }
    else if(cls->PrepareNewConn(fd, sfd, cfd))
    {
      if (cls->DBType == DBTypeMySQL)
      {
        conn = new MySQLConnection(proxy_id);
      }
	  else if (cls->DBType == DBTypePGSQL)
	  {
		  conn = new PgSQLConnection(proxy_id);
	  }
      cls->Server_cb(fd, which, arg, conn, sfd, cfd);
    }
}

void wrap_Proxy(int fd, short which, void * arg)
{
    int proxy_id;

    Connection * con = (Connection *) arg;
    proxy_id = con->iProxyId;

    logevent(NET_DEBUG, "[%d]frontend socket fired %d\n", proxy_id, fd);
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();

    if (conf->bRunning == false)
    {
      GreenSQL * cls = proxies[proxy_id];
      if (cls)
        cls->Close();
    }
    else
    {
      Proxy_cb(fd, which, arg);
    }

}

void wrap_Backend(int fd, short which, void * arg)
{
    int proxy_id;
    Connection * con = (Connection *) arg;
    proxy_id = con->iProxyId;

    logevent(NET_DEBUG, "[%d]backend socket fired %d\n", proxy_id, fd);

    GreenSQLConfig * conf = GreenSQLConfig::getInstance();

    if (conf->bRunning == false)
    {
      GreenSQL * cls = proxies[proxy_id];
      if (cls)
      {
        cls->Close();
      }
    }
    else
    {
      Backend_cb(fd, which, arg);
    }

}


bool proxymap_init()
{
    proxymap_reload();
    if (proxies.size() == 0)
        return false;
    
    return true;
}

bool proxymap_close()
{
    std::map<int, GreenSQL * >::iterator iter;
    GreenSQL * cls = NULL;

    while (proxies.size() != 0)
    {
        iter = proxies.begin();
        cls = iter->second;
        cls->Close();
        delete cls;
        proxies.erase(iter);
    }
    return true;
}

bool proxymap_reload()
{
    db_struct db;
    GreenSQL * cls = NULL;
    bool ret;

    std::string backendIP;
    int backendPort;
    std::string proxyIP;
    int proxyPort;
    int proxy_id = 99;
    std::string dbType = "mysql";
    
    backendIP = "127.0.0.1";
    backendPort = 3306;

    proxyIP = "127.0.0.1";
    proxyPort = 3305;

    /* read new urls from the database */
    if (! db_query(&db,q_proxy,256)) {
        logevent(STORAGE,"DB config erorr: %s\n",db_error());
        db_cleanup(&db);
        return false;
    } 

    /* Get a row from the results */
    while (db_fetch_row(&db))
    {
        proxy_id = db_col_int(&db,0);
        proxyIP = (char *) db_col_text(&db,1);
        proxyPort = db_col_int(&db,2);
        backendIP = (char *) db_col_text(&db,4);
        backendPort = db_col_int(&db,5);
        dbType = (char *) db_col_text(&db,6);
        //new object
        
        std::map<int, GreenSQL * >::iterator itr;
        itr = proxies.find(proxy_id);

        if (itr == proxies.end())
        {
            cls = new GreenSQL();

            bool ret = cls->ProxyInit(proxy_id, proxyIP, proxyPort, 
                backendIP, backendPort, dbType);
            if (ret == false)
            {
                // failed to init proxy
                proxymap_set_db_status(proxy_id, 2);
                cls->Close();
                delete cls;
            } else {
                proxymap_set_db_status(proxy_id, 1);
                proxies[proxy_id] = cls;
            }
            continue; 
        }

        // found proxy object
        cls = itr->second;

        // check if db settings has been changed
        if (cls->sProxyIP != proxyIP || cls->iProxyPort != proxyPort || cls->sDBType != dbType)
        {
            ret = cls->ProxyReInit(proxy_id, proxyIP, proxyPort,
                                   backendIP, backendPort, dbType);
            if (ret == false)
            {
                proxymap_set_db_status(proxy_id, 2);
            } else {
                proxymap_set_db_status(proxy_id, 1);
            }

            continue;
        } else if (cls->sBackendIP != backendIP ||
                   cls->iBackendPort != backendPort)
        {
            // may be backend ip has been changed
            cls->sBackendIP = backendIP;
            cls->iBackendPort = backendPort;
        }
    }

    if (cls == NULL)
    {
        // nothing found
        db_cleanup(&db);
        return false;
    }
    if (cls->ServerInitialized() == true)
    {
        proxymap_set_db_status(proxy_id, 1);
    } else {
        // we failed to open this server object, try once again
        ret = cls->ProxyReInit(proxy_id, proxyIP, proxyPort,
                               backendIP, backendPort, dbType);
        if (ret == false)
        {
            proxymap_set_db_status(proxy_id, 2);
        } else {
            proxymap_set_db_status(proxy_id, 1);
        }
    }

    /* Release memory used to store results. */
    db_cleanup(&db);  

    return true;
}

/*
 *  proxy status
 *  0 - reload
 *  1 - good
 *  2 - failed to load
 *
 */

bool proxymap_set_db_status(unsigned int proxy_id, int status )
{
    char query[1024];

    snprintf(query, sizeof(query), 
            "UPDATE proxy set status=%d where proxyid=%d",
            status, proxy_id);

	if ( ! db_exec(query))
	{
                logevent(STORAGE,"DB config erorr: %s\n",db_error());
		/* Make query */
		return false;
	}

    return true;
}
