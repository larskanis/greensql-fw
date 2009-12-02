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

static const char * q_db = "SELECT db_perm.dbpid, proxy.proxyid, db_perm.db_name, "
    "db_perm.perms, db_perm.perms2, db_perm.status, proxy.dbtype "
    "FROM db_perm, proxy WHERE proxy.proxyid=db_perm.proxyid";

static const char * q_default_db = "SELECT dbpid, proxyid, db_name, "
    "perms, perms2, status, sysdbtype FROM db_perm where proxyid=0";

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

DBPermObj * dbmap_default(int proxy_id, const char * const db_type)
{
    // try to find default database fot the specific db type
    std::map<std::string, DBPermObj * >::iterator itr;
    std::string key;
    DBPermObj * db;

    if (strcmp(db_type, "mysql") == 0)
    {
      key = "empty_mysql";
    } else if (strcmp(db_type, "pgsql") == 0)
    {
      key = "default_pgsql";
    }
    itr = dbs.find(key);
    if (itr != dbs.end())
    {
      db = itr->second;
      return db;
    }
    return default_db;
}

DBPermObj * dbmap_find(int proxy_id, std::string &db_name, const char * const db_type)
{
    std::map<std::string, DBPermObj * >::iterator itr;
    DBPermObj * db;
	
    std::string key;
    key = itoa(proxy_id);
    key += ",";
    key += db_name;

    itr = dbs.find(key);
    if (itr != dbs.end())
    {
      db = itr->second;
      return db;
    }
    // try to find default database for the specific db type
    if (strcmp(db_type, "mysql") == 0)
    {
      if (db_name.length() == 0)
        key = "empty_mysql";
      else
        key = "default_mysql";
    } else if (strcmp(db_type, "pgsql") == 0)
    {
       key = "default_pgsql";
    }
    itr = dbs.find(key);
    if (itr != dbs.end())
    {
      db = itr->second;
      return db;
    }
    return default_db;
}

bool dbmap_reload()
{
    std::map<std::string, DBPermObj * >::iterator itr;
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    db_struct db;

    char * q_learning3;
    char * q_learning7;
    //"SELECT dpid, db_perm.pid, db_name, "
    //"create_perm, drop_perm, alter_perm, "
    //"info_perm, block_q_perm "

    unsigned int proxy_id = 0;
    std::string db_name = "";
    std::string db_type = "";
    unsigned int status = 0;
    long long perms = 0;
    long long perms2 = 0;

    std::string key;

    if ( conf->sDbType == DB_MYSQL)
    {
        q_learning3 = "UPDATE db_perm SET status = 4 WHERE status = 11 AND now() > status_changed + INTERVAL 3 day";
        q_learning7 = "UPDATE db_perm SET status = 4 WHERE status = 12 AND now() > status_changed + INTERVAL 7 day";
    }
    else if ( conf->sDbType == DB_PGSQL)
    {
        q_learning3 = "UPDATE db_perm SET status = 4 WHERE status = 11 AND now() > status_changed + INTERVAL '3 day'";
        q_learning7 = "UPDATE db_perm SET status = 4 WHERE status = 11 AND now() > status_changed + INTERVAL '7 day'";
    }

    /* update state -> from learning to active protection */        
    if (!db_exec(q_learning3)) { logevent(STORAGE,"DB config erorr: %s\n",db_error()); return false; }
    if (!db_exec(q_learning7)) { logevent(STORAGE,"DB config erorr: %s\n",db_error()); return false; }

    /* read all database */
    if (!db_query(&db,q_db,256)) {
        logevent(STORAGE,"DB config erorr: %s\n",db_error());
        db_cleanup(&db);
        return false;
    } 

    /* Get a row from the results */
    while (db_fetch_row(&db))
    {
        proxy_id = db_col_int(&db,1);
        db_name = (char *) db_col_text(&db,2);
        perms = db_col_long_long(&db,3);
        perms2 = db_col_long_long(&db,4);
        status = db_col_int(&db,5);
        db_type = (char *) db_col_text(&db,6);

        key = itoa(proxy_id);
        key += ",";
        key += db_name;

        itr = dbs.find(key);
        if (itr == dbs.end())
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
    db_cleanup(&db);

    /* read default database */
    if (!db_query(&db,q_default_db,256)) {
        logevent(STORAGE,"DB config erorr: %s\n",db_error());
        db_cleanup(&db);
        return false;
    } 

    /* Get a row from the results */
    while (db_fetch_row(&db))
    {
        proxy_id = db_col_int(&db,1);
        db_name = (char *) db_col_text(&db,2);
        perms = db_col_long_long(&db,3);
        perms2 = db_col_long_long(&db,4);
        status = db_col_int(&db,5);
        // sysdbtype
        db_type = (char *) db_col_text(&db,6);

        if (strcmp(db_type.c_str(), "empty_mysql") == 0)
        {
          db_name = "";
        } else {
          db_name = db_type;
        }
        key = db_type;
        //str_lowercase(key);

        std::map<std::string, DBPermObj * >::iterator itr;
        itr = dbs.find(key);
        if (itr == dbs.end())
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
    db_cleanup(&db);

    return true;
}

