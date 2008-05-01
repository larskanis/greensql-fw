//
// GreenSQL SQL language parser.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef _SQL_PARSER_HPP_
#define _SQL_PARSER_HPP_

#include "expression.hpp"

struct query_risk {
	int has_separator;
	int has_tautology;
	int has_empty_pwd;
	int has_s_table;
	int has_comment;
	int has_union;
	int has_or;
};

bool query_parse(struct query_risk * q_risk, const char * q);

void clb_found_or_token();
void clb_found_union_token();
void clb_found_empty_pwd();
void clb_found_comment();
void clb_found_table(SQLString * t);
void clb_found_tautology();
void clb_found_query_separator();

#endif
