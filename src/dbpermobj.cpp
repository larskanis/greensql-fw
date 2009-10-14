//
// GreenSQL DB Permission class implementation.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "config.hpp"
#include "dbpermobj.hpp"
#include "misc.hpp"
#include "log.hpp"

void DBPermObj::Init(std::string name, unsigned int id, long long p,
		     long long perms2, unsigned int status)
{
    db_name = name;
    proxy_id = id;
    block_status = (DBBlockStatus) status;
    perms = p;

    if (perms == 0)
      return;
    if (perms & (int)CREATE_Q)
        create_perm = true;
    if (perms & (int)DROP_Q)
	drop_perm = true;
    if (perms & (int)ALTER_Q)
	alter_perm = true;
    if (perms & (int)INFO_Q)
	info_perm = true;
    if (perms & (int)BLOCK_Q)
	block_q_perm = true;
    if (perms & (int)BRUTEFORCE_Q)
	bruteforce_perm = true;
    return;
}

DBPermObj::~DBPermObj()
{
    exceptions.clear();
}

bool DBPermObj::LoadWhitelist()
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    MYSQL * dbConn = &conf->dbConn;
    //get records from the database
    MYSQL_RES *res; /* To be used to fetch information into */
    MYSQL_ROW row;
   
    char q[1024];
    snprintf(q, 1024, "SELECT query, perm FROM query "
		      "WHERE proxyid = %d AND db_name='%s'",
		      proxy_id, db_name.c_str());

    /* read new exceptions from the database */
    if( mysql_query(dbConn, q) )
    {
        /* Make query */
        logevent(STORAGE,"%s\n",mysql_error(dbConn));
        return false;
    }

    /* Download result from server */
    res=mysql_store_result(dbConn);
    if (res == NULL)
    {
        logevent(STORAGE, "SQL Exceptions Found: 0, error:%s\n", 
                 mysql_error(dbConn));
        return false;
    }
    //my_ulonglong mysql_num_rows(MYSQL_RES *result)
    //logevent(STORAGE, "SQL Exceptions Found: %lld\n", mysql_num_rows(res) );

    int perm = 0;
    std::string q_ex = "";

    std::map<std::string, int, std::less<std::string> > q_list;
	    
    /* Get a row from the results */
    while ((row=mysql_fetch_row(res)))
    {
        perm = atoi(row[1]);
        q_ex = row[0];
        if (perm > 0 && q_ex.length() > 0)
        {
            //q_list[q_ex] = perm;        
	    q_list.insert(std::pair<std::string, int>(q_ex, perm));
        }
    }
    /* Release memory used to store results. */
    mysql_free_result(res);
	
    exceptions.clear();
    exceptions = q_list;
    return true;
}

int DBPermObj::CheckWhitelist(std::string & q)
{
    std::map<std::string, int >::iterator itr;
    itr = exceptions.find(q);	
    if (itr == exceptions.end())
        return 0;
    return itr->second;
}

bool DBPermObj::AddToWhitelist(std::string & dbuser, std::string & pattern)
{
    char * q = new char [ pattern.length() + 1024 ];
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    MYSQL * dbConn = &conf->dbConn;
    MYSQL_RES *res; /* To be used to fetch information into */

    snprintf(q, pattern.length() + 1024,
        "SELECT queryid FROM query WHERE "
        "proxyid = %d and db_name = '%s' and query = '%s'",
        proxy_id, db_name.c_str(), pattern.c_str());

    /* read new queryid from the database */
    if( mysql_query(dbConn, q) )
    {
        /* Failed */
        logevent(STORAGE,"(%s) %s\n", q, mysql_error(dbConn));
        delete [] q;
        return false;
    }

    /* Download result from server */
    res=mysql_store_result(dbConn);
    if (res != NULL && mysql_num_rows(res) > 0)
    {
        // nothing to do here, query is aready in whitelist
        delete [] q;
        /* Release memory used to store results. */
        mysql_free_result(res);
        return true;
    } else if (res != NULL)
    {
        /* Release memory used to store results. */
        mysql_free_result(res);
    }

    // add pattern to the whitelist
    snprintf(q, pattern.length() + 1024,
        "INSERT into query "
        "(proxyid, perm, db_name, query) "
        "VALUES (%d,1,'%s','%s')",
        proxy_id, db_name.c_str(), pattern.c_str());

    /* read new urls from the database */
    if ( mysql_query(dbConn, q) )
    {
        /* Make query */
        logevent(STORAGE,"(%s) %s\n", q, mysql_error(dbConn));
        delete [] q;
        return false;
    }
    delete [] q;
    q = NULL;

    return true;
}

