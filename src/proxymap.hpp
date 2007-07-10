//
// GreenSQL functions used to manage a number of proxy objects.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#ifndef _GREENSQL_PROXYMAP_HPP_
#define _GREENSQL_PROXYMAP_HPP_

bool proxymap_init();
bool proxymap_reload();
bool proxymap_close();
void wrap_Server(int fd, short which, void * arg);
void wrap_Proxy(int fd, short which, void * arg);
void wrap_Backend(int fd, short which, void * arg);
bool proxymap_set_db_status(unsigned int proxy_id, int status );	

#endif
