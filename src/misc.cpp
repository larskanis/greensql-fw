//
// GreenSQL String functions code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include <sstream>
#include <string>

std::string itoa(const int & i)
{
    std::ostringstream o;
    if (!(o<<i))
	    return "ERROR";
    return o.str();
}

void TrimStr(std::string & str)
{
   // first remove comment
   size_t com = str.find("#");
   if (com != std::string::npos)
   {
       str = str.substr(0,com-1);
   }
   size_t start = str.find_first_not_of(" \t\r\n");
   size_t end = str.find_last_not_of(" #\t\r\n");

   if ((std::string::npos == start) ||
       (std::string::npos == end)     )
   {
      str = "";
      return;
   }
   str = str.substr(start, end+1);

}

bool ParseConfLine(std::string & str, std::string & key, std::string & value)
{
    size_t i = 0;
    std::string k = "";;
    std::string v = "";

    while ( i < str.size() && 
            ((str[i] >= 'a' && str[i] <= 'z') ||
             (str[i] >= 'A' && str[i] <= 'Z') ||
	     str[i] == '_'                      ) )
    {
        k += str[i];
	i++;
    }
    while ( i< str.size() && 
           (str[i] == ' ' || str[i] == '\t') )
    {
        i++;
    }
    if (str[i] != '=')
        return false;
    i++;
    while ( i< str.size() &&
           (str[i] == ' ' || str[i] == '\t') )
    {
        i++;
    }

    while ( i < str.size() &&
            ((str[i] >= 'a' && str[i] <= 'z') ||
             (str[i] >= 'A' && str[i] <= 'Z') ||
             (str[i] >= '0' && str[i] <= '9') ||
	     str[i] == '_' || str[i] == ':'   ||
	     str[i] == '/' || str[i] == '\\'  ||
	     str[i] == '.' || str[i] == '-' ) )
    {
        v += str[i];
        i++;
    }
    if (v.size() > 0 && k.size() > 0)
    {
        key = k;
        value = v;
        return true;
    }
    return false;
}

void str_lowercase(std::string &str)
{
    std::string::iterator i = str.begin();
    while (i != str.end()) {
        *i = tolower(*i);
        i++;
    }
}

