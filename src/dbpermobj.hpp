//
// GreenSQL DB Permission class header.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREENSQL_DBPERMOBJ_HPP
#define GREENSQL_DBPERMOBJ_HPP

#include <string>
#include <map>

class DBPermObj
{
public:
    DBPermObj(): create_perm(false), 
                 drop_perm(false),
                 alter_perm(false),
                 info_perm(false),
		 block_q_perm(false),
                 db_name("default"),
                 proxy_id(0)
    {}

    ~DBPermObj();

    void Init(std::string name, unsigned int id,
		    bool _create, bool _drop,
		    bool _alter, bool _info, bool _block_q);
    
    bool DBPermObj::LoadExceptions();
    int CheckQuery(std::string & q);
	    
    bool CanCreate()
    {
        return create_perm;
    }
    bool CanDrop()
    {
	    return drop_perm;
    }
    bool CanAlter()
    {
	    return alter_perm;
    }
    bool CanGetInfo()
    {
	    return info_perm;
    }
    bool CanBlockQueries()
    {
	    return block_q_perm;
    }
private: 
    bool create_perm;
    bool drop_perm;
    bool alter_perm;
    bool info_perm;
    bool block_q_perm;
    std::string db_name;
    std::string db_type;
    unsigned int proxy_id;
    std::map<std::string, int, std::less<std::string>  > exceptions;
    
};

#endif
