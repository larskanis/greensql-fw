//
// GreenSQL DB info retrieval functions.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "dbpermobj.hpp"
#include "config.hpp"
#include "dbmap.hpp"
#include "misc.hpp"
#include "log.hpp"

#include <map>
//#include <string>

static std::map<std::string, DBPermObj * > dbs;
static DBPermObj * default_db = NULL;

static const char * const q_db = "SELECT dbpid, proxyid, db_name, "
                                 "perms, perms2, status "
				 "FROM db_perm ";
// the following query is used to swith db from learning mode to active protection
static const char * const q_learning3 = "UPDATE db_perm SET status = 4 "
	"WHERE status = 11 AND now() + INTERVAL 3 day > status_changed";
static const char * const q_learning7 = "UPDATE db_perm SET status = 4 "
        "WHERE status = 11 AND now() + INTERVAL 7 day > status_changed";
	

bool dbmap_init()
{
    default_db = new DBPermObj(); 

    dbmap_reload();
    if (dbs.size() == 0)
        return false;
    return true;
}

bool dbmap_close()
{
    if (default_db != NULL)
    {
        delete default_db;
	default_db = NULL;
    }

    std::map<std::string, DBPermObj * >::iterator itr;
    while (dbs.size() != 0)
    {
        itr = dbs.begin();
        delete itr->second;
	dbs.erase(itr);
    }
    dbs.clear();
    return true;
}

DBPermObj * dbmap_default()
{
    return default_db;
}
DBPermObj * dbmap_find(int proxy_id, std::string &name)
{

    DBPermObj * db = NULL;
	
    std::string key;
    key = itoa(proxy_id);
    key += ",";
    key += name;
    str_lowercase(key); 

    std::map<std::string, DBPermObj * >::iterator itr;
    itr = dbs.find(key);
    if (itr == dbs.end())
        return default_db;
    db = itr->second;

    return db;
}

bool dbmap_reload()
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    MYSQL * dbConn = &conf->dbConn;
    //get records from the database
    MYSQL_RES *res; /* To be used to fetch information into */
    MYSQL_ROW row;
    //"SELECT dpid, db_perm.pid, db_name, "
    //"create_perm, drop_perm, alter_perm, "
    //"info_perm, block_q_perm "

    unsigned int proxy_id = 0;
    std::string db_name = "";
    unsigned int status = 0;
    long long perms = 0;
    long long perms2 = 0;

    std::string key;

    /* update state -> from learning to active protection */
    mysql_query(dbConn, q_learning3);
    mysql_query(dbConn, q_learning7);

    /* read new urls from the database */
    if( mysql_query(dbConn, q_db) )
    {
        /* Make query */
        logevent(STORAGE,"%s\n",mysql_error(dbConn));
        return false;
    }

    /* Download result from server */
    res=mysql_store_result(dbConn);
    if (res == NULL)
    {
        //logevent(STORAGE, "Records Found: 0, error:%s\n", mysql_error(dbConn));
        return false;
    }
    //my_ulonglong mysql_num_rows(MYSQL_RES *result)
    //logevent(STORAGE, "Records Found: %lld\n", mysql_num_rows(res) );

    /* Get a row from the results */
    while ((row=mysql_fetch_row(res)))
    {
        proxy_id = atoi(row[1]);
        db_name = row[2];
        perms = atoll(row[3]);
        perms2 = atoll(row[4]);
        status = atoi(row[5]);

        key = itoa(proxy_id);
        key += ",";
        key += db_name;
        str_lowercase(key);

        std::map<std::string, DBPermObj * >::iterator itr;
        itr = dbs.find(key);
	if (proxy_id == 0)
	{
            // default db
	    default_db->Init(db_name, proxy_id, perms, perms2, status);
	}
	else if (itr == dbs.end())
        {
            DBPermObj * db = new DBPermObj();

            db->Init(db_name, proxy_id, perms, perms2, status);
	    db->LoadWhitelist();
            dbs[key] = db;
        } else {
            //reload settings
            DBPermObj * db = itr->second;
            db->Init(db_name, proxy_id, perms, perms2, status);
            db->LoadWhitelist();
	}
    }
    /* Release memory used to store results. */
    mysql_free_result(res);

    return true;
}

