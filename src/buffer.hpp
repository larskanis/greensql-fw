//
// GreenSQL Buffer class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREEN_SQL_BUFFER_HPP
#define GREEN_SQL_BUFFER_HPP

#include <string>
#include <iostream>

class Buffer
{
private:
	std::string _buff;
public:
	Buffer();
	~Buffer();
	bool append(const char * data, int size);
	bool append(const std::string & s) { _buff.append(s); return true;}
	// remove some bytes at the begining of the buffer
	bool chop_back(int size);
	//
	bool pop(std::string & res, int size,bool appendval = false);
	//return raw data
	const unsigned char * raw();
	//find string in buffer
	unsigned int FindSubString(const std::string& value,unsigned int start = 0){return (unsigned)_buff.find(value,start);}
	int size();

};
#endif

