//
// GreenSQL SQL query normalization API.
// 
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef _GREEN_SQL_NORMALIZATION_HPP_
#define _GREEN_SQL_NORMALIZATION_HPP_
//#pragma once

bool normalizeQuery(std::string & query);
bool removeQuotedText(std::string & query);
bool removeComments(std::string & query);
bool removeSpaces(std::string & query);

#endif

