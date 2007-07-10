//
// GreenSQL Config class implementation.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//


#include "config.hpp"
#include "log.hpp"
#include "misc.hpp"

#include <iostream>
#include <fstream> //ifstream

GreenSQLConfig* GreenSQLConfig::_obj = NULL;

GreenSQLConfig::GreenSQLConfig()
{
	init();
}

GreenSQLConfig::~GreenSQLConfig()
{
	logevent(DEBUG, "config destructor\n");
}

GreenSQLConfig* GreenSQLConfig::getInstance()
{
	if (_obj == NULL)
	{
		_obj = new GreenSQLConfig();
		//_obj.load();
	}
	return _obj;
}
void GreenSQLConfig::free()
{
    if (_obj != NULL)
    {
	    _obj->close_db();
	    delete _obj;
	    _obj = NULL;
    }
}

void GreenSQLConfig::init()
{
    bSupport = true;

    bRunning = true;

    sDbHost = "127.0.0.1";
    iDbPort = 3306;
    sDbName = "greendb";
    sDbUser = "green";
    sDbPass = "green";

    re_block_level = 30;
    re_warn_level = 20;
    re_sql_comments = 30;
    re_s_tables = 10;
    re_or_token = 10;
    re_var_cmp_var = 30;
    re_always_true = 30;
    re_empty_password = 30;
    
    log_level = 3;
    log_file = "/var/log/greensql.log";
}

bool GreenSQLConfig::load(std::string & path)
{
    std::string cfg_file;
    cfg_file.reserve(1024); 

    cfg_file = path;
    cfg_file += "greensql.conf";
    
    std::ifstream file(cfg_file.c_str());
    std::string line;
    //std::string str;
    std::string key="";
    std::string value= "";
    std::string section = "";
    line.reserve(1024);

    if (!file.is_open())
    {
        //logevent(CRIT, "Failed to load configuration file: %s .\n",
        //                cfg_file.c_str());
        return false;
    }

    //load default settings
    init();

    while (std::getline(file, line))
    {
        //ignore empty lines
        if (line.length() == 0)
            continue;
        //ignore comments
        if (line[0] == '#' || line[0] == ';')
            continue;
        TrimStr(line);

        if (line[0] =='[')
        {
            size_t end_section = line.find(']');
            //check if we found end of section
            if (end_section == 1 || end_section == std::string::npos)
            {
                logevent(DEBUG, "Failed to parse following config line: %s\n",
                                line.c_str());
                return false;
            }
            section = line.substr(1, end_section-1);
	    str_lowercase(section);
            continue;
        }

        // format:
	// key=value
        if (ParseConfLine(line, key, value) == false)
	{
            logevent(CRIT, "Failed to parse following configuration line: %s.\n",
			    line.c_str());
	    return false;
	}
	str_lowercase(key);
        if (section == "database")
        {
            parse_db_setting(key, value);
        } else if (section == "risk engine")
	{
            parse_re_setting(key, value);
	} else if (section == "logging")
	{
	    parse_log_setting(key, value);
	}
	
    }
    return true;
}

bool GreenSQLConfig::parse_db_setting(std::string & key, std::string & value)
{
    if (key == "dbname")
    {
        sDbName = value;
    } else if (key == "dbuser")
    {
        sDbUser = value;
    } else if (key == "dbpass")
    {
       sDbPass = value;
    } else if (key == "dbhost")
    {
       sDbHost = value;
    } else if (key == "dbport")
    {
       iDbPort = atoi(value.c_str());
    }
    return true;
}

bool GreenSQLConfig::parse_re_setting(std::string & key, std::string & value)
{
    str_lowercase(key);
    if (key == "block_level")
    {
        re_block_level = atoi(value.c_str());
    } else if (key == "warn_level")
    {
        re_warn_level = atoi(value.c_str());
    } else if (key == "risk_sql_comments")
    {
        re_sql_comments = atoi(value.c_str());
    } else if (key == "risk_senstivite_tables")
    {
        re_s_tables = atoi(value.c_str());
    } else if (key == "risk_or_token")
    {
        re_or_token = atoi(value.c_str());
    } else if (key == "risk_var_cmp_var")
    {
        re_var_cmp_var = atoi(value.c_str());
    } else if (key == "risk_always_true")
    {
        re_always_true = atoi(value.c_str());
    } else if (key == "risk_empty_password")
    {
        re_empty_password = atoi(value.c_str());
    } 

    return true;
}

bool GreenSQLConfig::parse_log_setting(std::string & key, std::string & value)
{
    if (key == "loglevel")
    {
        log_level = atoi(value.c_str());
    } else if (key == "logfile")
    {
        log_file = value;
    }
    return true;	
}

bool GreenSQLConfig::load_db()
{
    if ( !mysql_init(&dbConn) )
    {
        logevent(DEBUG, "Mysql error: %s\n", mysql_error(&dbConn));
	return false;
    }

    if(!mysql_real_connect(&dbConn, sDbHost.c_str(), sDbUser.c_str(),
                           sDbPass.c_str(), sDbName.c_str(),
                           iDbPort, NULL, 0))
    {
        logevent(DEBUG, "Mysql error: %s\n", mysql_error(&dbConn));
        return false;
    }
    return true;
}

bool GreenSQLConfig::close_db()
{
    mysql_close(&dbConn);
    mysql_library_end();
    return true;
}

bool GreenSQLConfig::load_patterns(std::string & path)
{
     
    std::string file = path + "mysql.conf";

    return mysql_patterns.Load(file);
}
