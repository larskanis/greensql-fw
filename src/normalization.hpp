//
// GreenSQL SQL query normalization API.
// 
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//
#include "config.hpp"    // for DBProxyType

#ifndef _GREEN_SQL_NORMALIZATION_HPP_
#define _GREEN_SQL_NORMALIZATION_HPP_
//#pragma once

bool normalizeQuery(DBProxyType db_type,std::string & query);
bool removeQuotedText(DBProxyType db_type,std::string & query);
bool removeComments(std::string & query);
bool removeSpaces(std::string & query);

#endif

