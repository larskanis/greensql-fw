//
// GreenQSL configuration class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_CONFIG_HPP
#define GREEN_SQL_CONFIG_HPP

#include <string.h>         // fix Fedora 9 compilation errors.
#include <stdlib.h>         // fix Fedora 9 compilation errors.
#include <string>
#ifdef WIN32
#include <winsock2.h>
#else
#include <dlfcn.h>
#endif
#include "sql_api.hpp"

#ifdef WIN32
#define snprintf _snprintf
#endif
enum DBProxyType { DBTypeMySQL, DBTypePGSQL };
enum backend_db {DB_MYSQL = 1,DB_PGSQL};
class GreenSQLConfig
{
public:
    static GreenSQLConfig * getInstance();
    static backend_db ParseDbType(const std::string& type);
    static void free();
    bool bSupport;
    bool bRunning;
    bool load_db();
    bool close_db();
    bool load(std::string & path);

    // risk engine factors
    int re_block_level;
    int re_warn_level;
    int re_sql_comments;
    int re_s_tables;
    int re_or_token;
    int re_union_token;
    int re_var_cmp_var;
    int re_always_true;
    int re_empty_password;
    int re_multiple_queries;
    int re_bruteforce;

    int log_level;
    std::string log_file;
    backend_db sDbType;

private:
    size_t iProxyId;
    std::string sDbHost;
    int iDbPort;
    std::string sDbName;
    std::string sDbUser;
    std::string sDbPass;

    bool parse_db_setting(std::string & key, std::string & value);
    bool parse_re_setting(std::string & key, std::string & value);
    bool parse_log_setting(std::string & key, std::string & value);

    static GreenSQLConfig * _obj;
    GreenSQLConfig();
    ~GreenSQLConfig();
    void init();
};

#endif
