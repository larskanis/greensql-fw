//
// GreenSQL SQL expressions class.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef _SQL_EXPRESSION_HPP_
#define _SQL_EXPRESSION_HPP_

#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <iostream>   //cout
#include <iterator>   //ostream_iterator
#include <list>
#include <string.h>   // for string related functions like strlen

#ifdef WIN32
#define strcasecmp _stricmp
#endif

class SQLString {
public:
  SQLString(char * str)
  {
    int_value = strlen(str);
    str_value = str;

    sql_strings.push_front(this);
    itr = sql_strings.begin();    
    if (*itr != this)
    {
	    exit(1);
    }
  }
  SQLString(std::string & str): str_value(str)
  {
    int_value = str.length();

    sql_strings.push_front(this);
    itr = sql_strings.begin();
  }
  ~SQLString()
  {
    sql_strings.erase(itr);
  }
  const char * Dump()
  {
    return str_value.c_str();
  }
  size_t Length() const
  {
    return str_value.length();
  }
  const std::string & GetStr() const
  {
    return str_value;
  }  
private:
  int int_value;
  //std::string str_value;
  std::list<SQLString *>::iterator itr;
public:
  std::string str_value;
  static std::list<SQLString *> sql_strings;
};


class Expression {
public:
  enum ConstType 
  {
    Undef,
    Integer,
    String,
    Complex
  };
  Expression() : has_const(0), str_length(0), const_type(Undef)
  {
    //has_const = 0;
    //str_length = 0;
    //const_type = Undef;
    expressions.push_front(this);
    itr = expressions.begin();
  }
  ~Expression()
  {
    expressions.erase(itr);
  }
  void AddField(SQLString * str)
  {
    fields.insert(str->Dump()); 
    delete str;
  }
  void AddConst(SQLString * str)
  {
    has_const = 1; 
    if (const_type == Undef)
    {
      const_type = String;
      str_length = str->Length();
    } else if (const_type == String)
    {
      str_length += str->Length();
    }
    delete str;
  }
  void AddConst(int value)
  {
    has_const = 1;
    const_type = Integer;
  }
  void Add(Expression * xp2)
  {
    std::set<std::string>::iterator itr;
    itr = xp2->fields.begin();
    while (itr != xp2->fields.end())
    {
        fields.insert(*itr);
        itr++;
    }
    //fields = set_union(fields, xp2.fields);
    if (xp2->has_const == 1)
    {
      has_const = 1;
      if (xp2->const_type == String)
      {
        str_length = (str_length > xp2->str_length) ? str_length : xp2->str_length;
      }
    }
    delete xp2;
  }

  bool Comp(Expression * xp2)
  {
#ifdef PARSER_DEBUG
    std::cout << "Left : ";
    Dump();
    if (xp2 != NULL)
    {
      std::cout << "Right: ";
      xp2->Dump();
    }
#endif

    if (fields.size() != xp2->fields.size())
    {
      return false;
    }
    if (fields.size() == 0)
    {
      //compare const
      //std::cout << "true" << std::endl;
      return true;
    }
    //std::cout << "exp diff : ";
    std::vector<std::string> result;
    set_difference(fields.begin(), fields.end(), xp2->fields.begin(), xp2->fields.end(),
		    std::back_inserter(result));
    //std::ostream_iterator<std::string>(std::cout, " "));
    //std::cout << std::endl;
    if (result.size() == 0)
    {
      if (has_const == xp2->has_const)
      {
        //std::cout << "true" << std::endl;
	return true;
      }
    }
    return false;
  }
  
  bool IsTrue()
  {
    if (fields.size() == 0 && has_const)
    {
        //std::cout << "true" << std::endl;
	return true;
    }
    return false;
  }

  bool IsEmptyPwd(Expression * xp2)
  {
    if (xp2->const_type == String && xp2->str_length == 0 && xp2->fields.size() == 0 &&
	const_type == Undef && fields.size() == 1)
    {
      const char * str = (*(fields.begin())).c_str();
      if (strcasecmp(str, "pwd") == 0 ||
          strcasecmp(str, "pass") == 0 ||
	      strcasecmp(str, "password") == 0)
      {
        return true;
      }
    }
    return false;
  }

  void Dump()
  {
    std::set<std::string>::iterator itr;
    itr = fields.begin();
    
    std::cout << "exp cont: ";
    while (itr != fields.end())
    {
        std::cout << *itr << ", ";
	itr++;
    }
    std::cout << ". Has Const: ";
    if (has_const == 1)
    {
      std::cout << "1." << std::endl;
    } else {
      std::cout << "0." << std::endl;
    }

  }
private:
  std::set<std::string> fields;
  int has_const;  
  int str_length;
  ConstType const_type; 
  std::list<Expression *>::iterator itr;
public:
  static std::list<Expression *> expressions;
};

#endif
