//
// GreenSQL Buffer class implementation.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include <string>
#include <iostream>
#include "buffer.hpp"

Buffer::Buffer()
{
    //std::cout << "Buffer constructor" << std::endl;
    _buff.reserve(1024);
}

Buffer::~Buffer()
{
    //std::cout << "buffer destructor" << std::endl;
    _buff.clear();
}
    
bool Buffer::append(const char * data, int size)
{
        if (size <= 0)
        	return true;
        _buff.append(data, size);
        return true;
}

// remove some bytes at the begining of the buffer
bool Buffer::chop_back(int size)
{
        if (size<= 0)
        	return true;
        _buff.erase(0, size);
        return true;
}

bool Buffer::pop(std::string & res, int size,bool appendval)
{
	if(appendval)
		res.append( _buff.substr(0, size) );
	else
		res.assign( _buff, 0, size);
	_buff.erase(0, size);
	return true;
}

//return raw data
const unsigned char * Buffer::raw()
{
        return (const unsigned char *)_buff.c_str();
}

int Buffer::size()
{
        return (int)_buff.size();
}


