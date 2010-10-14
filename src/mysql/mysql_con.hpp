//
// GreenSQL MySQL Connection class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_MYSQL_CONNECTION_HPP
#define GREEN_SQL_MYSQL_CONNECTION_HPP

#include "../connection.hpp"
#include "../log.hpp"

bool mysql_patterns_init(std::string & path);

enum MySQLType {
    /* client requests */
    MYSQL_SLEEP           = 0x0,
    MYSQL_QUIT            = 0x1, 
    MYSQL_DB              = 0x2,
    MYSQL_QUERY           = 0x3, 
    MYSQL_FIELD_LIST      = 0x4,
    MYSQL_CREATE_DB       = 0x5,
    MYSQL_DROP_DB         = 0x6,
    MYSQL_REFRESH         = 0x7,
    MYSQL_SHUTDOWN        = 0x8,
    MYSQL_STATISTICS      = 0x9,
    MYSQL_PROCESS_INFO    = 0xa,
    MYSQL_CONNECT         = 0xb,
    MYSQL_KILL            = 0xc,
    MYSQL_DEBUG           = 0xd,
    MYSQL_PING            = 0xe,
    MYSQL_INT_TIME        = 0xf,
    MYSQL_DELAYED_INSERT     = 0x10,
    MYSQL_CHANGE_USER        = 0x11,
    MYSQL_REPL_BINLOG_DUMP   = 0x12,
    MYSQL_REPL_TABLE_DUMP    = 0x13,
    MYSQL_REPL_CONNECT_OUT   = 0x14,
    MYSQL_REPL_REGISTER_SLAVE = 0x15,
    MYSQL_STMT_PREPARE    = 0x16, //22, 
    MYSQL_STMT_EXEC       = 0x17, //23,
    MYSQL_LONG_DATA       = 0x18,
    MYSQL_STMT_CLOSE      = 0x19,
    MYSQL_STMT_RESET      = 0x1a,
    MYSQL_SET_OPTION      = 0x1b,
    MYSQL_STMT_FETCH      = 0x1c,

    /* server responses */
    MYSQL_SRV_OK             = 0x00,
    MYSQL_SRV_ENDROW         = 0xfe,
    MYSQL_SRV_ERROR          = 0xff
};

enum MySQLCap {
    MYSQL_CON_LONG_PWD        = 0x1,
    MYSQL_CON_FOUND_ROWS      = 0x2,
    MYSQL_CON_LONG_FLAG       = 0x4,
    MYSQL_CON_SELECT_DB       = 0x8,
    MYSQL_CON_NO_SCHEMA       = 0x0f,
    MYSQL_CON_COMPRESS        = 0x20,
    MYSQL_CON_ODBC            = 0x40,
    MYSQL_CON_USE_LOCAL_FILES = 0x80,
    MYSQL_CON_IGNORE_SPACE    = 0x100,
    MYSQL_CON_PROTOCOL_V41    = 0x200,
    MYSQL_CON_INTERACTIVE     = 0x400,
    MYSQL_CON_SSL             = 0x800,

	/* new flags supported from 4.1 */
    MYSQL_CON_IGNORE_SIGPIPE  = 0x1000,
    MYSQL_CON_TRANSACTIONS    = 0x2000,
    MYSQL_CON_RESERVED        = 0x4000,
    MYSQL_CON_SECURE_CONNECT  = 0x8000,
    MYSQL_CON_MULTI_STMT      = 0x10000,
    MYSQL_CON_MULTI_RESULTS   = 0x20000,
};

class MySQLConnection: public Connection
{
public:
    MySQLConnection(int id);
    ~MySQLConnection();
    bool checkBlacklist(std::string & query, std::string & reason);
    bool parseRequest(std::string & request, bool & hasResponse );
    bool parseResponse(std::string & response);
    bool blockResponse(std::string & response);
    SQLPatterns * getSQLPatterns();
	bool ParseRequestPacket(const unsigned char* data, size_t& request_size,bool& hasResponse);
	bool ParseResponsePacket(const unsigned char* data, size_t& response_size, size_t max_response_size,std::string& response,size_t& header_size);
    bool mysql41;
    bool longResponse;
    bool longResponseData;
    MySQLType lastCommandId;
	bool StartResponse;
};

#endif
