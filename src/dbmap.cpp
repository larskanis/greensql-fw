//
// GreenSQL DB info retrieval functions.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "dbpermobj.hpp"
#include "config.hpp"
#include "mysql/mysql_con.hpp"
#include "dbmap.hpp"
#include "misc.hpp"

#include <map>
//#include <string>

static std::map<std::string, DBPermObj * > dbs;
static DBPermObj * default_db = NULL;

static const char * const q_db = "SELECT dbpid, proxyid, db_name, "
                                 "create_perm, drop_perm, alter_perm, "
				 "info_perm, block_q_perm "
                                 "FROM db_perm ";
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
    std::string db_name;
    bool alter_b = false;
    bool create_b = false;
    bool drop_b = false;
    bool info_b = false;
    bool block_q_b = false;

    std::string key;

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
        
        if (*row[3] == '1')
            create_b = true;
        if (*row[4] == '1')
            drop_b = true;
        if (*row[5] == '1')
            alter_b = true;
        if (*row[6] == '1')
            info_b = true;
        if (*row[7] == '1')
            block_q_b = true;

        key = itoa(proxy_id);
        key += ",";
        key += db_name;
        str_lowercase(key);

        std::map<std::string, DBPermObj * >::iterator itr;
        itr = dbs.find(key);
	if (proxy_id == 0)
	{
            // default db
	    default_db->Init(db_name, proxy_id, create_b, drop_b,
			     alter_b, info_b, block_q_b);
	}
	else if (itr == dbs.end())
        {
            DBPermObj * db = new DBPermObj();

            db->Init(db_name, proxy_id, create_b, drop_b,
                    alter_b, info_b, block_q_b); 
	    db->LoadExceptions();
            dbs[key] = db;
        } else {
            //reload settings
            DBPermObj * db = itr->second;
            db->Init(db_name, proxy_id, create_b, drop_b,
                    alter_b, info_b, block_q_b);
            db->LoadExceptions();
	}
    }
    /* Release memory used to store results. */
    mysql_free_result(res);

    return true;
}

