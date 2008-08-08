//
// GreenSQL risk engine API. This is GreenSQL core functions.
// These functions are intended to find SQL tautologies.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

// THIS CODE IS NO LONGER IN USE.

/*
#include "riskengine.hpp"
#include "normalization.hpp"
#include "config.hpp"
#include "misc.hpp"
#include "log.hpp"
#include "parser/parser.hpp"

static bool check_comments(std::string & query);
//static bool check_sensitive_tables(std::string & query);
//static bool check_sensitive_table(std::string & table);
static bool check_or_token(std::string & query);
static bool check_union_token(std::string & query);
//static bool check_empty_password(std::string & query);
//
// This function calculates risk assosicated with specific SQL query.
// For more information about this file, please check docs/tautology.txt
//

*/
/*
unsigned int calc_risk(std::string & query, std::string & pattern, 
		std::string & reason)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    unsigned int ret = 0;

    struct query_risk risk;
    query_parse(&risk, query.c_str());

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

*/

/*
static bool check_or_token(std::string & query)
{
    size_t p = 1;
    while ( (p = query.find("or", p)) != std::string::npos)
    {
        if (query[p-1] != ' ' && query[p-1] != ')' && query[p-1] != '?')
	{
            p += 2;
            continue;
	}
	if (query[p+2] != ' ' && query[p+2] != '(' && query[p+2] != '?')
	{
            p += 2;
	    continue;
	}
	//std::string test = query.substr(p-1, query.size() -p);
	//logevent(DEBUG, "OR Token: %s\n", test.c_str());
	return true;
    }
    return false;
}

*/

/*
static bool check_union_token(std::string & query)
{
    size_t p = 1;
    while ( (p = query.find("union", p)) != std::string::npos)
    {
        if (query[p-1] != ' ' && query[p-1] != ')' && query[p-1] != '?')
        {
            p += 5;
            continue;
        }
        if (query[p+5] != ' ')
        {
            p += 5;
            continue;
        }
        //std::string test = query.substr(p-1, query.size() -p);
        //logevent(DEBUG, "OR Token: %s\n", test.c_str());
        return true;
    }
    return false;
}
*/

/*
static bool check_comments(std::string & query)
{
   size_t p = 0;
   std::string temp = query;
   removeQuotedText(temp);

   // we need to check if query has comments
   if ( (p = temp.find("/*",0)) != std::string::npos)
   {
       return true;
   } else if ( (p = temp.find("//",0)) != std::string::npos)
   {
       return true;
   } else if (  (p = temp.find("--",0)) != std::string::npos)
   {
       return true;
   } else if ( (p = temp.find("#",0)) != std::string::npos)
   {
       return true;
   }
   
   return false;
}

*/

/*
static bool check_sensitive_tables(std::string & query)
{
    // we need parse query
    // SELECT xxx from tables where
    // SELECT xxx from t1 left join t2 on XXX and XXX left join
    // left/inner/right join
    // case is not important here - everything must go lowecase ??? 
    std::string list;
    size_t start;
    size_t end;
    start = query.find(" from ", 0);

    if (start == std::string::npos)
    {
        return false;
    }
    start += 6;
    end = query.find(" where ", start);
    if (end == std::string::npos)
    {
        end = query.size();
    } else {
    }
    
    list = query.substr(start, end - start);
    
    // convers word "join" to a comma
    start = 0;
    while ( (start = list.find(" join ",start)) != std::string::npos)
    {
        list.erase(start+1, 4);
	list[start] = ',';
    }
    
    start = 0;
    size_t temp = 0;
    std::string table;
    end = 0;
    while ( (end = list.find(",", end)) != std::string::npos)
    {
       if (list[start] == ' ')
	       start++;
       temp = list.find(" ", start);
       if (temp != std::string::npos && temp < end)
       {
          table = list.substr(start, temp - start); 
       } else if (temp != std::string::npos)
       {
          table = list.substr(start, end - start);
       } else {
	  table = list.substr(start, end - start);
       }
       if ( check_sensitive_table(table) == true)
           return true;   
       end++;
       start = end;
    }
    // get last table
    if ( start < list.size())
    {
       if (list[start] == ' ')
           start++;
       temp = list.find(" ", start);
       table = list.substr(start, temp-start);
       if ( check_sensitive_table(table) == true)
           return true;
    }
	   
    return false;
}

static bool check_sensitive_table(std::string & table)
{
    if (table.size() == 0)
        return false;
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    
    return conf->mysql_patterns.Match( SQL_S_TABLES,  table );
}

*/

/*
static bool check_empty_password(std::string & query)
{
    // SELECT xxx FROM XX WHERE XXX and password = ''
    //                                  passwor AND/OR
    //                                  password = ""
    //                                  password )
    std::string temp = query;
    removeComments(temp);
    removeSpaces(temp);
    str_lowercase(temp);

    size_t p = temp.find(" where ", 0);
    if (p == std::string::npos)
    {
        return false;
    }
    p+= 5;

    temp = temp.substr(p, temp.size() - p);

    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    return conf->mysql_patterns.Match( SQL_EMPTY_PWD,  temp );
}
*/

