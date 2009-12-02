//
// GreenSQL String functions code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//
#include "misc.hpp"
#include <sstream>

#define BUFF_SIZE 4096
std::string itoa(const int & i)
{
    std::ostringstream o;
    if (!(o<<i))
	    return "ERROR";
    return o.str();
}
std::string ltoa(const long & i)
{
    std::ostringstream o;
    if (!(o<<i))
        return "ERROR";
    return o.str();
}

std::string lltoa(const long long & i)
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
    // Another varian for this function is to do the following:
    // transform(tmp.begin(), tmp.end(), tolower);
    std::string::iterator i = str.begin();
    while (i != str.end()) {
        *i = tolower(*i);
        i++;
    }
}
void CutStringToVector(const std::string& str,std::vector<std::string>& vec,char sep)
{
    int oi,i;
    for (oi=0 ; (i=str.find(sep,oi))>=0 ; )
    {
        vec.push_back(str.substr(oi,i-oi));
        oi=i+1;
    }
    if((str.size()-oi) >0)
        vec.push_back(str.substr(oi));
}
std::string TrimString(std::string& value,char sep)
{
    unsigned int start=0,end = value.size();
    bool checkFirst = true,checkLast = true;
    std::string::iterator iter = value.begin();
    std::string::reverse_iterator eiter = value.rbegin();
    for(;iter != value.end() && eiter != value.rend() &&(checkFirst || checkLast);++iter,++eiter)
    {
        if(checkFirst)
        {
            if(*iter != sep)
                checkFirst = false;
            else
                start++;
        }
        if(checkLast)
        {
            if(*eiter != sep)
                checkLast = false;
            else
                end--;
        }
    }
    return value.substr(start,end - start);
}
std::string TrimRightString(std::string& value,char sep)
{
    unsigned int end = value.size();
    bool checkLast = true;
    std::string::reverse_iterator eiter = value.rbegin();
    for(;eiter != value.rend() && checkLast;++eiter)
    {
        if(*eiter != sep)
            checkLast = false;
        else
            end--;
    }
    return value.substr(0,end);
}
std::string TrimLeftString(std::string& value,char sep)
{
    unsigned int start=0;
    bool checkFirst = true;
    std::string::iterator iter = value.begin();
    for(;iter != value.end() && checkFirst;++iter)
    {
        if(*iter != sep)
            checkFirst = false;
        else
            start++;
    }
    return value.substr(start,value.size() - start);
}
