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
	MYSQL_QUIT        = 1, 
	MYSQL_DB          = 2,
	MYSQL_QUERY       = 3, 
	MYSQL_FIELDLIST   = 4,
	MYSQL_STATISTICS  = 9,
	MYSQL_PROCESSINFO = 10,
	MYSQL_PREPARE     = 22, 
	MYSQL_EXEC        = 23,
	MYSQL_ENDROW      = 0xfe
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

	std::string serverVersion;
	int charset;
        int serverCaps;
	int clientCaps;
	bool longResponse;
	bool longResponseData;
	MySQLType lastCommandId;
};

#endif
