//
// GreenSQL event logging API.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_LOG_HPP
#define GREEN_SQL_LOG_HPP

#include <string>

enum ErrorType { CRIT, ERR, INFO, SQL_DEBUG, DEBUG, STORAGE, NET_DEBUG};

bool log_init(std::string & file, int level);
bool log_close();
void logevent(ErrorType type, const char * fmt, ...);
void loghex(ErrorType type, const unsigned char * data, int size);

#endif

