//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREENSQL_LOGALERT_HPP
#define GREENSQL_LOGALERT_HPP

#include <string>

bool logalert(int proxy_id, std::string & dbname, std::string & dbuser,
		std::string & query, std::string & pattern, 
		std::string & reason, int risk, int block);


#endif
