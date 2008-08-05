//
// GreenSQL SQL language parser.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include <iostream>
#include "expression.hpp"
#include "parser.hpp"

#ifndef PARSER_DEBUG
#include "../config.hpp"
#endif

std::list<SQLString *> SQLString::sql_strings;
std::list<Expression *> Expression::expressions;

int scan_buffer(const char * data);
static bool free_sql_strings();
static bool free_expressions();

static query_risk risk;
static query_risk * p_risk = &risk;

#ifdef PARSER_DEBUG
// ind debug mode we have our own main function
int main()
{
  char buf[1024];
  snprintf(buf,sizeof(buf), "SELECT * FROM users WHERE laston=unix_timestamp()+10*20+fn() AND user_level>1 ORDER BY userid ASC");
  scan_buffer(buf);

  /*
  snprintf(buf,sizeof(buf), "select 1 from user where 1 -- commnent");
  scan_buffer(buf);
      
  std::string s;
  while (std::cin)
  {
    std::getline(std::cin, s);
    //std::cout << "line: " << s << std::endl;
    scan_buffer(s.c_str());
  }
  */
  free_sql_strings();
  free_expressions();
  return 1;
}

#endif

bool query_parse(struct query_risk * q_risk, const char * q)
{
  p_risk = q_risk;
  //bzero(q_risk, sizeof(struct query_risk));
  memset(q_risk, sizeof(struct query_risk), 0);
  scan_buffer(q);
  free_sql_strings();
  free_expressions();
  return true;
}

void clb_found_or_token()
{
  p_risk->has_or = 1;  
}

void clb_found_union_token()
{
  p_risk->has_union = 1;
}

void clb_found_empty_pwd()
{
  p_risk->has_empty_pwd = 1;
}

void clb_found_comment()
{
  p_risk->has_comment = 1;
}

void clb_found_table(SQLString * s)
{
  if (*(s->Dump()) == 0)
    return;
#ifndef PARSER_DEBUG
  GreenSQLConfig * conf = GreenSQLConfig::getInstance();
  if (conf->re_s_tables >= 0 &&
      conf->mysql_patterns.Match( SQL_S_TABLES,  * s->GetStr() ) )
    p_risk->has_s_table = 1; 
#else
  p_risk->has_s_table = 1;
#endif
}

bool clb_check_true_constant(SQLString * s)
{
  if (*(s->Dump()) == 0)
    return false;
#ifndef PARSER_DEBUG
  GreenSQLConfig * conf = GreenSQLConfig::getInstance();
  if (conf->re_s_tables >= 0 &&
      conf->mysql_patterns.Match( SQL_S_TABLES,  * s->GetStr() ) )
    return true;
#endif
  return false;
}
	
void clb_found_tautology()
{
  p_risk->has_tautology = 1;
}

void clb_found_query_separator()
{
  p_risk->has_separator = 1;
}


static bool free_sql_strings()
{
  std::list<SQLString *>::iterator itr;
  for (itr  = SQLString::sql_strings.begin();
       itr != SQLString::sql_strings.end();
       itr  = SQLString::sql_strings.begin())
  {
    delete *itr;
  }
  return true;
}

static bool free_expressions()
{
  std::list<Expression *>::iterator itr;
  for (itr  = Expression::expressions.begin();
       itr != Expression::expressions.end();
       itr  = Expression::expressions.begin())
  {
    delete *itr;
  }
  return true;
}

