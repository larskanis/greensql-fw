//
// GreenSQL String functions API.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREENSQL_MISC_HPP
#define GREENSQL_MISC_HPP

std::string itoa(const int & i);
void TrimStr(std::string & str);
bool ParseConfLine(std::string & str, std::string & key, std::string & value);
void str_lowercase(std::string & str);

#endif
