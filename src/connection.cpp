//
// GreenSQL DB Connection class implementation.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "connection.hpp"
#include "greensql.hpp"
#include "normalization.hpp"
#include "riskengine.hpp"
#include "misc.hpp"
#include "dbmap.hpp"
#include "alert.hpp"


Connection::Connection(int proxy_id)
{
    iProxyId = proxy_id;
    logevent(NET_DEBUG, "connection init()\n");
    first_request = true;
    db = dbmap_default();
    db_name = "";
    db_new_name = "";
    db_user = "";
}

bool Connection::close()
{
    logevent(NET_DEBUG, "connection close()\n");
    GreenSQL::socket_close(proxy_event.ev_fd);
    GreenSQL::socket_close(client_event.ev_fd);
    if (event_initialized(&proxy_event))
        event_del(&proxy_event);
    if (event_initialized(&client_event))
        event_del(&client_event);
    return true;
}

bool Connection::check_query(std::string & query)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    std::string original_query = query;
    std::string reason = "";

    normalizeQuery(query);
    str_lowercase(query);
    std::string pattern = query;

    logevent(SQL_DEBUG, "AFTER NORM   : %s\n", query.c_str());
    bool ret = false;
    int risk = 0;
    
    if (db->CanAlter() == false)
    {
        ret = conf->mysql_patterns.Match( SQL_ALTER,  query );
	if (ret == true)
	{
            reason += "Query blocked due to the 'alter' rules.\n";
	}
    }
    if (ret == false && db->CanDrop() == false)
    {
	ret = conf->mysql_patterns.Match( SQL_DROP,  query );
        if (ret == true)
        {
            reason += "Query blocked due to the 'drop' rules.\n";
        }
    }
    if (ret == false && db->CanCreate() == false)
    {
        ret = conf->mysql_patterns.Match( SQL_CREATE,  query );
        if (ret == true)
        {
            reason += "Query blocked due to the 'create' rules.\n";
        }
    }
    if (ret == false && db->CanGetInfo() == false)
    {
        ret = conf->mysql_patterns.Match( SQL_INFO,  query );
        if (ret == true)
        {
            reason += "Query blocked due to the 'info' rules.\n";
        }
    }

    if (ret == false)
    {
	// if
	risk = calc_risk(original_query, query, reason);
	logevent(SQL_DEBUG, "RISK         : %d\n", risk);

	//check if risk is low
    } else if (ret == true)
    {
	risk = conf->re_block_level+1;
	logevent(SQL_DEBUG, "FORBIDEN     : %s\n", query.c_str());
    }
    
    int block = db->CheckQuery(pattern);
    if (block )
    {
	logevent(SQL_DEBUG, "Found in Exception List.\n");
	return true;
    }
    if (risk != 0)
    {
	if (risk >= conf->re_block_level)
	{
            // block level
            logalert(iProxyId, db_name, db_user, original_query, 
                     pattern, reason, risk, 1);
	    // block this query
	    return false;
	}
	else
	{
            //warn level
            logalert(iProxyId, db_name, db_user, original_query,
                     pattern, reason, risk, 0);
	}
    }

    return true;
}

