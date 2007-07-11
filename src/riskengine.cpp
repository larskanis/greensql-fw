//
// GreenSQL risk engine API. This is GreenSQL core functions.
// These functions are intended to find SQL tautologies.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "riskengine.hpp"
#include "normalization.hpp"
#include "config.hpp"
#include "misc.hpp"
#include "log.hpp"

static bool check_comments(std::string & query);
static bool check_sensitive_tables(std::string & query);
static bool check_sensitive_table(std::string & table);
static bool check_or_token(std::string & query);
static bool check_empty_password(std::string & query);
static bool check_var_cmp_var(std::string & query);
static bool check_always_true(std::string & query);

//
// This function calculates risk assosicated with specific SQL query.
// For more information about this file, please check docs/tautology.txt
//

unsigned int calc_risk(std::string & query, std::string & pattern, 
		std::string & reason)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    unsigned int ret = 0;

    if (conf->re_sql_comments > 0 && 
        check_comments(query) == true)
    {
	reason += "Query has comments\n";
        logevent(DEBUG, "Query has comments\n");
	ret += conf->re_sql_comments;
    }
    if (conf->re_s_tables > 0 && 
        check_sensitive_tables(pattern) == true)
    {
	reason += "Query uses sensitive tables\n";
        logevent(DEBUG, "Query uses sensitive tables\n");
	ret += conf->re_s_tables;
    }

    size_t p = pattern.find(" where ", 0);
    if (p == std::string::npos)
    {
        return ret;
    }

    // query has WHERE statement
    //pattern = pattern.substr(p+7, pattern.size() - p -7);
    //std::string where_str = pattern.substr(p+1, pattern.size() - p -1);
    
    std::string where_str(pattern, p+1, pattern.size() - p -1);
    //logevent(SQL_DEBUG, "WHERE: %s\n", where_str.c_str());

    if (conf->re_or_token > 0 &&
        check_or_token(where_str) == true)
    {
	reason += "Query has 'or' token\n";
	logevent(DEBUG, "Query has 'or' token\n");
	ret += conf->re_or_token;
    }
    if (conf->re_var_cmp_var > 0 && 
        check_var_cmp_var(where_str) == true)
    {
	reason += "Variable comparison only\n";
        logevent(DEBUG, "Variable comparison only\n");
	ret += conf->re_var_cmp_var;
    }
    if (conf->re_empty_password > 0 &&
        check_empty_password(where_str) == true)
    {
	reason += "Query has empty password value\n";
        logevent(DEBUG, "Query has empty password tokey\n");
	ret += conf->re_empty_password;
    }
    if (conf->re_always_true > 0 &&
        check_always_true(where_str) == true)
    {
	reason += "Query can always return true\n";
        logevent(DEBUG, "Query can always return true\n");
	ret += conf->re_always_true;
    }
    return ret;
}

static bool check_or_token(std::string & query)
{
    size_t p = 1;
    while ( (p = query.find("or", p)) != std::string::npos)
    {
        if (query[p-1] != ' ' && query[p-1] != ')')
	{
            p += 2;
            continue;
	}
	if (query[p+2] != ' ' && query[p+2] != '(')
	{
            p += 2;
	    continue;
	}
	std::string test = query.substr(p-1, query.size() -p);
	//logevent(DEBUG, "OR Token: %s\n", test.c_str());
	return true;
    }
    return false;
}

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
    
    p = 1;
    // ((and\s|or\s|not\s|)|(\(\s*))
    // (password|pwd|pass)
    // (\sand|\snot|\sor|\s?\))
    // \s*=\s*(''|"")

    while ( (p = temp.find("password", p)) != std::string::npos ||
            (p = temp.find("pass", p)) != std::string::npos ||
	    (p = temp.find("pwd", p)) != std::string::npos)
    {
        if (temp[p-1] != ' ' && temp[p-1] != ')')
        {
            p += 2;
            continue;
        }
	// check if before password we got and/not/or/(
	if (temp[p-1] == ' ' && 
	    !(temp[p-2] == '(' || temp[p-2] == 'r' || 
	     temp[p-2] == 't' || temp[p-2] == 'd') )
	{
            p+= 3;
	    continue;
	}
	if (temp.find("password", p) == p)
	{
	    p += 8;
	} else if (temp.find("pass", p) == p) {
	    p += 4;
	} else {
            // pwd
	    p += 3;
	}
	
        if (temp[p] == ')')
	{
	    return true;
	}
	
        if (temp[p] == ' ' && 
            (temp[p] == ')' || temp[p] == 'a' ||
             temp[p] == 'n' || temp[p] == 'o'))
	{
            return true;
	}
        while (temp[p] == ' ')
            p++;
 
        if (temp[p] == '=')
        {
            p += 1;
        }
        while (temp[p] == ' ')
            p++;

        if (temp[p] == '\'' && temp[p+1] == '\'')
            return true;
        if (temp[p] == '\"' && temp[p+1] == '\"')
            return true;

    }
    
    return false;
}

static bool check_var_cmp_var(std::string & query)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    return conf->mysql_patterns.Match( SQL_VAR_CMP, query );
}

static bool check_always_true(std::string & query)
{
    GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    return conf->mysql_patterns.Match( SQL_TRUE_VAR, query );
}
