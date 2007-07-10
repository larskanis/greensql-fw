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

void DBPermObj::Init(std::string name, unsigned int id,
                    bool _create, bool _drop,
                    bool _alter, bool _info, bool _block_q)
{
    db_name = name;
    proxy_id = id;
    create_perm = _create;
    drop_perm = _drop;
    alter_perm = _alter;
    info_perm = _info;
    block_q_perm = _block_q;
}

DBPermObj::~DBPermObj()
{
    exceptions.clear();
}

bool DBPermObj::LoadExceptions()
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
    logevent(STORAGE, "SQL Exceptions Found: %lld\n", mysql_num_rows(res) );

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

int DBPermObj::CheckQuery(std::string & q)
{
    std::map<std::string, int >::iterator itr;
    itr = exceptions.find(q);	
    if (itr == exceptions.end())
        return 0;
    return itr->second;
}
