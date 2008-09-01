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

enum DBBlockStatus {
    RISK_BLOCK          = 0,      // block based on the risk calculation 
    PRIVILEGE_BLOCK     = 1,      // block only admin and sensitive commands
    RISK_SIMULATION     = 2,      // simulation mode for risk calculation
    PRIVILEGE_SIMULATION= 3,      // simulation mode for sensitive commands
    ALWAYS_BLOCK_NEW    = 4,      // block all new queries
    LEARNING_MODE       = 10,      // automatically approve all queries
    LEARNING_MODE_3DAYS = 11,      //    ... for 3 days
    LEARNING_MODE_7DAYS = 12,      //    ... for 7 days
};

enum DBBlockLevel {
    WARN                = 0,
    BLOCKED             = 1,
    HIGH_RISK           = 2,
};

class DBPermObj
{
public:
    DBPermObj(): create_perm(false), 
                 drop_perm(false),
                 alter_perm(false),
                 info_perm(false),
		 block_q_perm(false),
		 bruteforce_perm(false),
                 block_status(RISK_BLOCK),
                 db_name("default"),
                 proxy_id(0)
    {}

    ~DBPermObj();

    void Init(std::string name, unsigned int id,
		    bool _create, bool _drop,
		    bool _alter, bool _info, bool _block_q);
    
    bool LoadExceptions();
    int CheckWhitelist(std::string & q);
	    
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
    DBBlockStatus GetBlockStatus()
    {
	    return block_status;
    }
private: 
    bool create_perm;
    bool drop_perm;
    bool alter_perm;
    bool info_perm;
    bool block_q_perm;
    bool bruteforce_perm;
    DBBlockStatus block_status;
    std::string db_name;
    std::string db_type;
    unsigned int proxy_id;
    std::map<std::string, int, std::less<std::string>  > exceptions;
    
};

#endif
