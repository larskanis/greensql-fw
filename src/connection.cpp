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
#include "parser/parser.hpp"     // for query_risk struct


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
    if (proxy_event.ev_fd != 0 && proxy_event.ev_fd != -1 && 
		proxy_event.ev_flags & EVLIST_INIT)
        event_del(&proxy_event);
    if (client_event.ev_fd != 0 && client_event.ev_fd != -1 && 
		client_event.ev_flags & EVLIST_INIT)
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
    ret = checkBlacklist(query, reason);

    if (ret == false)
    {
	// if
	risk = calculateRisk(original_query, query, reason);
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

unsigned int Connection::calculateRisk(std::string & query,
  std::string & pattern, std::string & reason)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    unsigned int ret = 0;

    struct query_risk risk;
    query_parse(&risk, getSQLPatterns(), query.c_str());

    if (conf->re_sql_comments > 0 &&
        risk.has_comment == 1)
    {
        reason += "Query has comments\n";
        logevent(DEBUG, "Query has comments\n");
        ret += conf->re_sql_comments;
    }

    if (conf->re_s_tables > 0 &&
        risk.has_s_table == 1)
    {
        reason += "Query uses sensitive tables\n";
        logevent(DEBUG, "Query uses sensitive tables\n");
        ret += conf->re_s_tables;
    }
    if (conf->re_multiple_queries > 0 &&
        risk.has_separator == 1)
    {
        reason += "Multiple queries found\n";
        logevent(DEBUG, "Multiple queries found\n");
        ret += conf->re_multiple_queries;
    }

    if (conf->re_or_token > 0 &&
        risk.has_or == 1)
    {
        reason += "Query has 'or' token\n";
        logevent(DEBUG, "Query has 'or' token\n");
        ret += conf->re_or_token;
    }
    if (conf->re_union_token > 0 &&
        risk.has_union == 1)
    {
        reason += "Query has 'union' token\n";
        logevent(DEBUG, "Query has 'union' token\n");
        ret += conf->re_union_token;
    }

    if (conf->re_var_cmp_var > 0 &&
        risk.has_tautology == 1)
    {
        reason += "Variable comparison only\n";
        logevent(DEBUG, "Variable comparison only\n");
        ret += conf->re_var_cmp_var;
    }
    if (conf->re_empty_password > 0 &&
        risk.has_empty_pwd == 1)
    {
        reason += "Query has empty password expression\n";
        logevent(DEBUG, "Query has empty password expression\n");
        ret += conf->re_empty_password;
    }
    return ret;
}

