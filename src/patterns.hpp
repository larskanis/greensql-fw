//
// GreenSQL SQL patterns class definition.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREENSQL_PATTERNS_HPP
#define GREENSQL_PATTERNS_HPP

#include <string>
#include <pcre.h>

enum MatchType { SQL_ALTER, SQL_CREATE, SQL_DROP, 
	         SQL_INFO, SQL_BLOCK, SQL_S_TABLES,
                 SQL_EMPTY_PWD, SQL_VAR_CMP, 
		 SQL_TRUE_CONSTANTS, SQL_BRUTEFORCE_FUNCTIONS };

class SQLPatterns
{
public:
    SQLPatterns();
    ~SQLPatterns();
    bool Load(std::string & file);
    void LoadPattern(std::string & section, std::string & line);
    bool CompilePatterns();
    bool Match(MatchType type, const std::string & str);
    bool HasTrueConstantPatterns() { return true_constants_re != NULL; }
    bool HasBruteforcePatterns() { return bruteforce_functions_re != NULL; }

private:
    void clear();
    void free();
    bool compile_pattern(std::string & str, pcre ** _re, pcre_extra ** _pe );
    
    std::string alter_str;
    std::string create_str;
    std::string drop_str;
    std::string info_str;
    std::string block_str;
    std::string s_tables_str; //used to store list of sensitive tables
    std::string empty_pwd_str;
    std::string var_cmp_str;
    std::string true_constants_str;
    std::string bruteforce_functions_str;

    //regular expression patterns 

    pcre * alter_re;
    pcre * create_re;
    pcre * drop_re;
    pcre * info_re;
    pcre * block_re;
    pcre * s_tables_re;
    pcre * empty_pwd_re;
    pcre * var_cmp_re;
    pcre * true_constants_re;
    pcre * bruteforce_functions_re;

    pcre_extra * alter_pe;
    pcre_extra * create_pe;
    pcre_extra * drop_pe;
    pcre_extra * info_pe;
    pcre_extra * block_pe;
    pcre_extra * s_tables_pe;
    pcre_extra * empty_pwd_pe;
    pcre_extra * var_cmp_pe;
    pcre_extra * true_constants_pe;
    pcre_extra * bruteforce_functions_pe;

};

#endif
