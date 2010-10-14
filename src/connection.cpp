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
#include "alert.hpp"
#include "parser/parser.hpp"     // for query_risk struct
#include "dbmap.hpp"

Connection::Connection(int proxy_id)
{
    iProxyId = proxy_id;
    logevent(NET_DEBUG, "connection init()\n");
    first_request = true;
    db = NULL;
    db_name = "";
    db_new_name = "";
    db_user = "";
    db_type = "";
    db_user_ip = "";
	SecondPacket = false;
	first_response = true;
}

bool Connection::close()
{
#ifndef WIN32
    logevent(NET_DEBUG, "connection close(), proxy socket %d, backend socket %d\n", 
              proxy_event.ev_fd, backend_event.ev_fd);
    GreenSQL::socket_close(proxy_event.ev_fd);
    GreenSQL::socket_close(backend_event.ev_fd);
    if (proxy_event.ev_fd != 0 && proxy_event.ev_fd != -1 && proxy_event.ev_flags & EVLIST_INIT)
        event_del(&proxy_event);
    if (proxy_event_writer.ev_fd != 0 && proxy_event_writer.ev_fd != -1 &&
                proxy_event_writer.ev_flags & EVLIST_INIT && proxy_event_writer.ev_flags & EVLIST_INSERTED)
        event_del(&proxy_event_writer);

    if (backend_event.ev_fd != 0 && backend_event.ev_fd != -1 && backend_event.ev_flags & EVLIST_INIT)
        event_del(&backend_event);
    if (backend_event_writer.ev_fd != 0 && backend_event_writer.ev_fd != -1 &&
                backend_event_writer.ev_flags & EVLIST_INIT && backend_event_writer.ev_flags & EVLIST_INSERTED )
        event_del(&backend_event_writer);
    connections->erase(location);
#endif
    return true;
}

bool Connection::check_query(std::string & query)
{
    //return true;
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    std::string original_query = query;
    std::string reason = "";

    // convert query to lower case
    str_lowercase(query);
    // perform normalization - make a pattern of the query
    normalizeQuery( (DBProxyType) dbType, query);
    // we will make the reference out of query
    std::string & pattern = query;

    logevent(SQL_DEBUG, "AFTER NORM   : %s\n", pattern.c_str());

    bool ret = false;
    bool privileged_operation = false;
    int risk = 0;

    // Optimization Changes
    // In case we are in automatic block of the new commands
    // we do not need to calculate query risk if the query
    // pattern is known and it is in the whitelist.
    // This must be 99% of the cases (after learning mode is over).

    int in_whitelist = 0;

    in_whitelist = db->CheckWhitelist(pattern);
    if ( in_whitelist )
    {
        logevent(SQL_DEBUG, "Found in Exception List.\n");
        return true;
    }

    if ( (ret = checkBlacklist(pattern, reason)) == true)
    {
         privileged_operation = true;
	 logevent(SQL_DEBUG, "FORBIDEN     : %s\n", pattern.c_str());
    }
    // check if we find anything interesting
    risk = calculateRisk(original_query, reason);
    logevent(SQL_DEBUG, "RISK         : %d\n", risk);

    DBBlockStatus block_status = db->GetBlockStatus();

    if ( block_status == ALWAYS_BLOCK_NEW )
    {
        reason += "Query blocked because it is not in whitelist.\n";
        logevent(DEBUG, "Query blocked because it is not in whitelist.\n");
        logalert(iProxyId, db_name, db_user, db_user_ip, original_query,
                 pattern, reason, risk, (int)BLOCKED);
        // block this query
        return false;
    }

    if (privileged_operation)
    {
	risk = conf->re_block_level+1;
        if (block_status == PRIVILEGE_BLOCK || block_status == RISK_BLOCK)
        {
            logalert(iProxyId, db_name, db_user, db_user_ip, original_query,
                     pattern, reason, risk, (int)BLOCKED);
            // block this query
            return false;
	} else if (block_status == LEARNING_MODE ||
		   block_status == LEARNING_MODE_3DAYS ||
		   block_status == LEARNING_MODE_7DAYS)
	{
	    db->AddToWhitelist(db_user, pattern);
            logwhitelist(iProxyId, db_name, db_user, db_user_ip, original_query,
                         pattern, reason, risk, (int)HIGH_RISK);
	    return true;
        } else {
            // block_status == RISK_SIMULATION 
            // block_status == LEARNING_MODE
            logalert(iProxyId, db_name, db_user, db_user_ip, original_query,
                     pattern, reason, risk, (int)HIGH_RISK);
            return true;
        }
    }

    if (block_status == LEARNING_MODE ||
        block_status == LEARNING_MODE_3DAYS ||
        block_status == LEARNING_MODE_7DAYS)
    {
        db->AddToWhitelist(db_user, pattern);
        if (risk >= conf->re_block_level)
        {
            logwhitelist(iProxyId, db_name, db_user, db_user_ip, original_query,
                     pattern, reason, risk, (int)HIGH_RISK);
        } else if (risk >= conf->re_warn_level)
        {
            logwhitelist(iProxyId, db_name, db_user, db_user_ip, original_query,
                     pattern, reason, risk, (int)WARN);
        } else {
            logwhitelist(iProxyId, db_name, db_user, db_user_ip, original_query,
                     pattern, reason, risk, (int)LOW);
	}

        return true;
    }

    DBBlockLevel risk_block_level = HIGH_RISK;
    if (block_status == RISK_BLOCK)
    {
        risk_block_level = BLOCKED;
    }

    if (risk >= conf->re_block_level) 
    {
        logalert(iProxyId, db_name, db_user, db_user_ip, original_query,
                 pattern, reason, risk, (int)risk_block_level);
        if (risk_block_level == BLOCKED)
            return false;
    } 
    else if (risk >= conf->re_warn_level)
    {
        //warn level
        logalert(iProxyId, db_name, db_user, db_user_ip, original_query,
                 pattern, reason, risk, (int)WARN);
    }

    return true;
}

unsigned int Connection::calculateRisk(std::string & query,
  std::string & reason)
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
        reason += "True expression detected (SQL tautology)\n";
        logevent(DEBUG, "True expression detected (SQL tautology)\n");
        ret += conf->re_var_cmp_var;
    }
    if (conf->re_empty_password > 0 &&
        risk.has_empty_pwd == 1)
    {
        reason += "Query has empty password expression\n";
        logevent(DEBUG, "Query has empty password expression\n");
        ret += conf->re_empty_password;
    }
    if (conf->re_bruteforce > 0 &&
        risk.has_bruteforce_function == 1)
    {
        reason += "Query has SQL fuction that can be used to bruteforce db contents\n";
	logevent(DEBUG, "Query has SQL fuction that can be used to bruteforce db contents\n");
	ret += conf->re_bruteforce;
    }
    return ret;
}

