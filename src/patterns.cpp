//
// GreenSQL SQL patterns class implementation.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "log.hpp"
#include <fstream>
#include "patterns.hpp" 
#include "misc.hpp"

SQLPatterns::SQLPatterns()
{
    clear();
}

SQLPatterns::~SQLPatterns()
{
    free();
}

void SQLPatterns::clear()
{
    alter_str    = "";
    create_str   = "";
    drop_str     = "";
    info_str     = "";
    block_str    = "";
    s_tables_str = "";
    empty_pwd_str= "";
    var_cmp_str  = "";
    true_constants_str= "";
    bruteforce_functions_str="";

    alter_re     = NULL;
    create_re    = NULL;
    drop_re      = NULL;
    info_re      = NULL;
    block_re     = NULL;
    s_tables_re  = NULL;
    empty_pwd_re = NULL;
    var_cmp_re   = NULL;
    true_constants_re= NULL;
    bruteforce_functions_re=NULL;

    alter_pe     = NULL;
    create_pe    = NULL;
    drop_pe      = NULL;
    info_pe      = NULL;
    block_pe     = NULL;
    s_tables_pe  = NULL;
    empty_pwd_pe = NULL;
    var_cmp_pe   = NULL;
    true_constants_pe=NULL;
    bruteforce_functions_pe=NULL;
}

void SQLPatterns::free()
{
    if (alter_pe != NULL)
       pcre_free(alter_pe);
    if (create_pe != NULL)
       pcre_free(create_pe);
    if (drop_pe != NULL)
       pcre_free(drop_pe);
    if (info_pe != NULL)
       pcre_free(info_pe);
    if (block_pe != NULL)
       pcre_free(block_pe);
    if (s_tables_pe != NULL)
       pcre_free(s_tables_pe);
    if (empty_pwd_pe != NULL)
       pcre_free(empty_pwd_pe);
    if (var_cmp_pe != NULL)
       pcre_free(var_cmp_pe);
    if (true_constants_pe != NULL)
       pcre_free(true_constants_pe);
    if (bruteforce_functions_pe != NULL)
       pcre_free(bruteforce_functions_pe);
 
    if (alter_re != NULL)
       pcre_free(alter_re);
    if (create_re != NULL)
       pcre_free(create_re);
    if (drop_re != NULL)
       pcre_free(drop_re);
    if (info_re != NULL)
       pcre_free(info_re);
    if (block_re != NULL)
       pcre_free(block_re);
    if (s_tables_re != NULL)
       pcre_free(s_tables_re);
    if (empty_pwd_re != NULL)
       pcre_free(empty_pwd_re);
    if (var_cmp_re != NULL)
       pcre_free(var_cmp_re);
    if (true_constants_re != NULL)
       pcre_free(true_constants_re);
    if (bruteforce_functions_re != NULL)
       pcre_free(bruteforce_functions_re);
    
    clear();
}

bool SQLPatterns::Load(std::string & cfg_file)
{
    std::ifstream file(cfg_file.c_str());
    std::string temp;

    if (!file.is_open())
    {
        logevent(CRIT, "Failed to load file with patterns - %s.\n", 
			cfg_file.c_str());
	return false;
    }
    std::string line = "";
    std::string section = "";
    std::string value = "";
    size_t end_section = 0;

    while (std::getline(file, line))
    {
        //ignore empty lines
	if (line.length() == 0)
            continue;
	//ignore comments
        if (line[0] == '#' || line[0] == ';')
            continue;

	if (line[0] =='[')
	{
	    end_section = line.find(']');
	    //check if we found end of section
            if (end_section == 1 || end_section == std::string::npos)
	    {
                logevent(DEBUG, "Failed to parse following config line: %s\n",
				line.c_str());
                return false;
	    }
            section = line.substr(1, end_section-1);
	    continue;
	}
        TrimStr(line);
	if (line.length()== 0)
            continue;

        if (section.size() == 0)
	{
            logevent(DEBUG, "Failed to parse following config line: %s . "
			    "Section is not specified.\n",
	                   line.c_str());
	    return false;
	}
        LoadPattern(section, line);
    }
    
    if (true_constants_str.length() > 0)
    {
      temp =  "^(";
      temp += true_constants_str;
      temp += ")$";
      true_constants_str = temp;
    }
    if (bruteforce_functions_str.length() > 0)
    {
      temp = "^(";
      temp += bruteforce_functions_str;
      temp += ")$";
      bruteforce_functions_str = temp;
    }

    if (s_tables_str.length() > 0)
    {
      temp = "(" + s_tables_str + ")";
      s_tables_str = temp;
    }
    empty_pwd_str = "((where\\s|and\\s|or\\s|not\\s|^)|(\\(\\s*))?"
                   "(pass|password|pwd|passwd)\\s*\\=\\s*(\\'\\')|(\\\"\\\")";

    var_cmp_str = "((where\\s|and\\s|or\\s|not\\s|^)|(\\(\\s*))?\\?\\s*(>|!)?\\=\\s*\\?";
	   
    CompilePatterns();
    return true;
}

void SQLPatterns::LoadPattern(std::string & section, std::string & line)
{
    // build regular expression pattern
    if (section == "alter")
    {
        if (alter_str.length() > 0)
            alter_str += "|";
        alter_str += line;
    } else if (section == "create")
    {
        if (create_str.length() > 0)
            create_str += "|";
        create_str += line;
    } else if (section == "drop")
    {
        if (drop_str.length() > 0)
            drop_str += "|";
        drop_str += line;
    } else if (section == "info")
    {
        if (info_str.length() > 0)
            info_str += "|";
        info_str += line;
    } else if (section == "block")
    {
        if (block_str.length() > 0)
            block_str += "|";
        block_str += line;
    } else if (section == "sensitive tables")
    {
/*
        size_t len = s_tables_str.size();
        if (s_tables_str.length() == 0)
        {
            s_tables_str = "^(";
        } else {
            s_tables_str.erase(len -2, len); //clear ")$"
            s_tables_str += "|";
        }
        s_tables_str += line;
        s_tables_str += ")$";
*/
	if (s_tables_str.length() > 0)
            s_tables_str += "|";
	s_tables_str += line;
    } else if (section == "true constants")
    {
        if (true_constants_str.length() > 0)
            true_constants_str += "|";
	true_constants_str += line;
    } else if (section == "bruteforce functions")
    {
        if (bruteforce_functions_str.length() > 0)
            bruteforce_functions_str += "|";
        bruteforce_functions_str += line;	
    }
}

bool SQLPatterns::CompilePatterns()
{
    if (compile_pattern(alter_str, &alter_re, &alter_pe) == false)
    {
	free();
        return false;
    }
    if (compile_pattern(drop_str, &drop_re, &drop_pe) == false)
    {
	free();
        return false;
    }
    if (compile_pattern(info_str, &info_re, &info_pe) == false)
    {
	free();
	return false;
    }
    if (compile_pattern(create_str, &create_re, &create_pe) == false)
    {
	free();
        return false;
    }
    if (compile_pattern(block_str, &block_re, &block_pe) == false)
    {
	free();
        return false;
    }
    if (compile_pattern(s_tables_str, &s_tables_re, &s_tables_pe) == false)
    {
	free();
	return false;
    }
    if (compile_pattern(empty_pwd_str, &empty_pwd_re, &empty_pwd_pe) == false)
    {
        free();
        return false;
    }

    if (compile_pattern(var_cmp_str, &var_cmp_re, &var_cmp_pe) == false)
    {
        free();
        return false;
    }

    if (compile_pattern(true_constants_str, &true_constants_re, &true_constants_pe) == false)
    {
        free();
        return false;
    }

    if (compile_pattern(bruteforce_functions_str, &bruteforce_functions_re, &bruteforce_functions_pe) == false)
    {
        free();
	return false;
    }

    return true;
}

bool SQLPatterns::compile_pattern(std::string & str, pcre ** _re, 
                                  pcre_extra ** _pe )
{
    const char * error;
    int erroffset;

    if (str.length() > 0)
    {
        *_re = pcre_compile(str.c_str(), PCRE_CASELESS,
                            &error, &erroffset, NULL );
        if (_re == NULL)
        {
            logevent(CRIT, "Failed to compile the following pattern %s. Error %s",
                            str.c_str(), error);
            return false;
        }
        *_pe = pcre_study( *_re, 0, &error);
    }
    return true;
}


bool SQLPatterns::Match(MatchType type, const std::string & str)
{
    int rc;
    unsigned int len = str.length();

    switch (type)
    {
        case SQL_ALTER:
        {
            if (alter_str.length() == 0)
                return false;
            rc = pcre_exec(alter_re, alter_pe, str.c_str(), len, 
			    0, 0, NULL, 0);
	    break;
        }
        case SQL_BLOCK:
        {
            if (block_str.length() == 0)
                return false;
            rc = pcre_exec(block_re, block_pe, str.c_str(), len,
                            0, 0, NULL, 0);
            break;
        }
        case SQL_CREATE:
        {
            if (create_str.length() == 0)
                return false;
            rc = pcre_exec(create_re, create_pe, str.c_str(), len,
                            0, 0, NULL, 0);
            break;
        }

        case SQL_DROP:
        {
            if (drop_str.length() == 0)
                return false;
            rc = pcre_exec(drop_re, drop_pe, str.c_str(), len,
                            0, 0, NULL, 0);
            break;
        }

        case SQL_INFO:
        {
            if (info_str.length() == 0)
                return false;
            rc = pcre_exec(info_re, info_pe, str.c_str(), len,
                            0, 0, NULL, 0);
            break;
        }

        case SQL_S_TABLES:
        {
            if (s_tables_str.length() == 0)
                return false;
            rc = pcre_exec(s_tables_re, s_tables_pe, str.c_str(), len,
                            0, 0, NULL, 0);
            break;
        }
        
	case SQL_EMPTY_PWD:
	{
            if (empty_pwd_str.length() == 0)
                return false;
	    rc = pcre_exec(empty_pwd_re, empty_pwd_pe,str.c_str(),len,
			   0, 0, NULL, 0);
            break;
	}

        case SQL_VAR_CMP:
        {
            if (var_cmp_str.length() == 0)
                return false;
            rc = pcre_exec(var_cmp_re, var_cmp_pe, str.c_str(), len,
                           0, 0, NULL, 0);
            break;
        }

        case SQL_TRUE_CONSTANTS:
        {
            if (true_constants_str.length() == 0)
                return false;
            rc = pcre_exec(true_constants_re, true_constants_pe,
                           str.c_str(), len,
                           0, 0, NULL, 0);
            break;

        }

        case SQL_BRUTEFORCE_FUNCTIONS:
        {
            if (bruteforce_functions_str.length() == 0)
                return false;
            rc = pcre_exec(bruteforce_functions_re, bruteforce_functions_pe,
                           str.c_str(), len,
                           0, 0, NULL, 0);
            break;

        }

	
	default:
	    return false;
    }
    // not found
    if (rc <0)
    {
        return false;
    }
    return true;
}

