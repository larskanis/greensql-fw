//
// GreenSQL MySQL Connection class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_PGSQL_CONNECTION_HPP
#define GREEN_SQL_PGSQL_CONNECTION_HPP

#include "../connection.hpp"
#include "../log.hpp"
#include <set>
using namespace std;

bool pgsql_patterns_init(std::string & path);

enum PgSQLType {
    /* client requests */  
    PGSQL_NOTICE_RESPONSE        = 0x4E,
    PGSQL_NO_DATA                = 0x6E,
    PGSQL_BIND                   = 0x42,
    PGSQL_PARSE_COMPLETE         = 0x1,
    PGSQL_BIND_COMPLETE          = 0x2,
    PGSQL_CLOSE_COMPLETE         = 0x3,
    PGSQL_NOTIF_RESPONSE         = 0x41,
    PGSQL_CLOSE                  = 0x43,
    PGSQL_DESCRIBE               = 0x44,
    PGSQL_EXECUTE                = 0x45,
    PGSQL_FUNC_CALL              = 0x46,
    PGSQL_COPY_IN_RESPONSE       = 0x47,
    PGSQL_FLUSH                  = 0x48,
    PGSQL_EMPTY_QUERY_RESPONSE   = 0x49, 
    PGSQL_PARSE                  = 0x50,
    PGSQL_QUERY                  = 0x51,
    PGSQL_SYNC                   = 0x53,
    PGSQL_FUN_CALL_RESPONSE      = 0x56,
    PGSQL_QUIT                   = 0x58,
    PGSQL_COPY_DONE              = 0x63,
    PGSQL_COPY_DATA              = 0x64,
    PGSQL_COPY_FAIL              = 0x66,
    PGSQL_PORTAL_SUSPENDED       = 0x73,

    /* server responses */
    PGSQL_SRV_PASSWORD_MESSAGE   = 0x52,
    PGSQL_AUTH_MD5               = 0xC,
    PGSQL_AUTH_OK                = 0x52,//82
    PGSQL_SRV_GETROW             = 0x54,//84
    PGSQL_SRV_ENDROW             = 0xfe,//254
    PGSQL_SRV_ERROR              = 0xff,//255
    PGSQL_ROW_DATA               = 0x44,//68
    PGSQL_PARAM_STATUS           = 0x53,//83
    PGSQL_BECKEND_KEY_DATA       = 0x4B,//75
    PGSQL_READY_FOR_QUERY        = 0x5A,//90 
    PGSQL_COMMAND_COMPLETE       = 0x43,//67
    PGSQL_ERROR_RESPONSE         = 0x45,//69
    //password type
    PGSQL_PASSWORD               = 0x70,
};

enum PgSQLCap {
    PGSQL_CON_COMPRESS        = 0x20,
    PGSQL_CON_PROTOCOL_V41    = 0x200,
    PGSQL_CON_SSL             = 0x1000,
};

class PgSQLConnection: public Connection
{
public:
    typedef set<string> strmap;
    PgSQLConnection(int id);
    ~PgSQLConnection();
    bool checkBlacklist(string & query, string & reason);
    bool parseRequest(string & request, bool & hasResponse );
    bool parseResponse(string & response);
    bool blockResponse(string & response);
    void blockParseResponse(string & response);
    SQLPatterns * getSQLPatterns();
    std::string serverVersion;
    int charset;
    int serverCaps;
    int clientCaps;
    bool longResponse;
    bool longResponseData;
    std::string proto_version;		//protocol version for postgre sql
	std::string db_options;
	bool dbcach_cach_response_is_full;
    bool FindErrorInParse(const string& value){return parseerror.find(value) != parseerror.end();}
    void SetParseError(const string& value){parseerror.insert(value);}
    void ClearParseError(const string& value){parseerror.erase(value);}
private:
	bool parse_request(const unsigned char * data, size_t full_size,bool & hasResponse);
	bool parse_response(const unsigned char * data, size_t& response_size, size_t& max_response_size, std::string & response);
    strmap parseerror;
};

#endif
