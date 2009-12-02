#ifndef GREEN_SQL_STRUCT
#define GREEN_SQL_STRUCT

#ifdef HAVE_MYSQL_CLIENT
#include "mysql.h"
#endif

#ifdef HAVE_PGSQL_CLIENT
#include "libpq-fe.h"
#endif

 // ResultSet
typedef struct {
    union
    {
#ifdef HAVE_MYSQL_CLIENT
        MYSQL_RES *res; /* To be used to fetch information into */
#endif

#ifdef HAVE_PGSQL_CLIENT
        PGresult* pgres;
#endif
    };

#ifdef HAVE_MYSQL_CLIENT
    MYSQL_ROW row;
#endif

#ifdef HAVE_PGSQL_CLIENT
    unsigned int maxpgrows; //pgsql only
    unsigned int curpgrow; //pgsql only
#endif
} db_struct;

#endif
