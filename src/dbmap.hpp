//
// GreenSQL DB info retrieval header.
// 
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef GREENSQL_DB_PERM_HPP
#define GREENSQL_DB_PERM_HPP


bool dbmap_init();

bool dbmap_reload();
bool dbmap_close();
DBPermObj * dbmap_default(int proxy_id, const char * const db_type);
DBPermObj * dbmap_find(int pid, std::string &name, const char * const db_type);


#endif

