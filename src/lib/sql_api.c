#include <string.h>
#include <stdlib.h>
#include "sql_struct.h"
#include "sql_api.h"

#ifdef HAVE_MYSQL_CLIENT
#include "mysql.h"
#endif

#ifdef HAVE_PGSQL_CLIENT
#include "libpq-fe.h"
#endif

#ifdef HAVE_MYSQL_CLIENT
MYSQL mysqldbConn;
#endif

#ifdef HAVE_PGSQL_CLIENT
PGconn* pgsqldbConn = NULL;
PGresult* pgres_temp = NULL;        //postgresql usage only result per query
#endif

int db_col_int(db_struct *db, int col)
{
#ifdef HAVE_MYSQL_CLIENT
    return atoi(db->row[col]);
#endif

#ifdef HAVE_PGSQL_CLIENT
    return atoi(PQgetvalue(db->pgres, db->curpgrow-1, col));
#endif

    return 0;
}

long long db_col_long_long(db_struct *db, int col)
{
#ifdef HAVE_MYSQL_CLIENT
    return atoll(db->row[col]);
#endif

#ifdef HAVE_PGSQL_CLIENT
    return atoll(PQgetvalue(db->pgres, db->curpgrow-1, col));
#endif
    return 0;
}

const char *db_col_text(db_struct *db, int col)
{
#ifdef HAVE_MYSQL_CLIENT
    return (char *)(db->row[col]);
#endif

#ifdef HAVE_PGSQL_CLIENT
    return PQgetvalue(db->pgres, db->curpgrow-1, col);
#endif

    return NULL;
}

#ifdef HAVE_PGSQL_CLIENT
int pgsql_fetch_row(db_struct *db)
{
    if(!db->pgres)
        return 0;
    if(db->curpgrow == db->maxpgrows)
        return 0;
     ++db->curpgrow;
    return 1;
}
#endif

int db_fetch_row(db_struct *db)
{
#ifdef HAVE_MYSQL_CLIENT
    if ((db->row=mysql_fetch_row(db->res))) {
        return 1;
    }
#endif

#ifdef HAVE_PGSQL_CLIENT
    if(pgsql_fetch_row(db))
        return 1;
#endif

    return 0;
}




char *db_escape_string(const char *str,unsigned int length)
{
    char *q = NULL;
    q = (char *) malloc(strlen(str)*4+1);

#ifdef HAVE_MYSQL_CLIENT
    mysql_real_escape_string(&mysqldbConn, q, str, (unsigned long) length);
    q[strlen(str)*4] = '\0';
#endif

#ifdef HAVE_PGSQL_CLIENT
    {
        int error;
        PQescapeStringConn(pgsqldbConn,q,str,length,&error);
        q[strlen(str)*4] = '\0';
    }
#endif

    return q;
}

int db_query(db_struct *db, const char *q,int size)
{
#ifdef HAVE_MYSQL_CLIENT
    if( mysql_query(&mysqldbConn, q) )
    {
        db->res = NULL;
        return 0;
    }
    
    db->res=mysql_store_result(&mysqldbConn);
    if (db->res == NULL)
    {
        // no records found - it is not an error
        return 1;
    }
    return 1;
#endif

#ifdef HAVE_PGSQL_CLIENT
    ExecStatusType restype;
    db->pgres = PQexec(pgsqldbConn,q);
    if(!db->pgres)
    {
        db->maxpgrows = db->curpgrow = 0;
        return 0;
    }
    restype = PQresultStatus(db->pgres);

    if (restype == PGRES_EMPTY_QUERY)
    {
        PQclear(db->pgres);
        db->pgres = NULL;
        db->maxpgrows = db->curpgrow = 0;
        return 1;
    }
    if (restype == PGRES_BAD_RESPONSE || restype == PGRES_FATAL_ERROR)
    {
        PQclear(db->pgres);
        db->pgres = NULL;
        db->maxpgrows = db->curpgrow = 0;
        return 0;
    }
    db->maxpgrows = PQntuples(db->pgres);
    db->curpgrow = 0;
    return 1;
#endif

    return 0;
}



int db_exec(const char *q)
{
#ifdef HAVE_MYSQL_CLIENT
    int rc;

    rc = mysql_query(&mysqldbConn, q);
    if (rc) { 
        return 0;
    }

    return 1;
#endif

#ifdef HAVE_PGSQL_CLIENT
    ExecStatusType restype;
    if (pgres_temp)
    {
        PQclear(pgres_temp);
        pgres_temp = NULL;
    }
    pgres_temp = PQexec(pgsqldbConn,q);
    if (!pgres_temp)
    {
            return 0;
    }
    restype = PQresultStatus(pgres_temp);

    if (restype == PGRES_BAD_RESPONSE || restype == PGRES_FATAL_ERROR)
    {
        PQclear(pgres_temp);
        pgres_temp = NULL;
    }
        
    return 1;
#endif

    return 0;
}

int db_changes()
{

#ifdef HAVE_MYSQL_CLIENT
    if (mysql_affected_rows(&mysqldbConn) )
    {
        // found
        return 1;
    }
#endif

#ifdef HAVE_PGSQL_CLIENT
    if (pgres_temp)
    {
        ExecStatusType type;
        if ((type = PQresultStatus(pgres_temp)) == PGRES_COMMAND_OK)
        {
            char* effectedrows = PQcmdTuples(pgres_temp);
            if(effectedrows && atoi(effectedrows) > 0) //found
                return 1;
        }
    }
#endif

    return 0;
}

void db_cleanup(db_struct *db)
{    
#ifdef HAVE_PGSQL_CLIENT
    if(db->pgres)
        PQclear(db->pgres);
    if(pgres_temp)
        PQclear(pgres_temp);
    db->maxpgrows = db->curpgrow = 0;
    db->pgres = NULL;
    pgres_temp = NULL;
#endif

#ifdef HAVE_MYSQL_CLIENT
    if (db->res)
    {
      mysql_free_result(db->res);
      db->res = NULL;
    }
#endif

}

int db_close()
{
#ifdef HAVE_MYSQL_CLIENT
    mysql_close(&mysqldbConn);
    mysql_library_end();
#endif

#ifdef HAVE_PGSQL_CLIENT
    PQfinish(pgsqldbConn);
#endif
    return 1;
}

int db_load(const char *sDbHost, const char *sDbUser, const char *sDbPass, const char *sDbName, int iDbPort)
{

#ifdef HAVE_MYSQL_CLIENT
    if ( !mysql_init(&mysqldbConn) )
    {
        return 0;
    }

#if MYSQL_VERSION_ID >= 50013
    my_bool trueval = 1;
    mysql_options(&mysqldbConn, MYSQL_OPT_RECONNECT, &trueval);
#endif

    if(!mysql_real_connect(&mysqldbConn, sDbHost, sDbUser, sDbPass, sDbName, iDbPort, NULL, 0))
    {
        return 0;
    }
#endif

#ifdef HAVE_PGSQL_CLIENT
    pgsqldbConn = NULL;
    pgres_temp = NULL;

    char connectInfo[10240];
    snprintf(connectInfo, sizeof(connectInfo),
        "hostaddr = %s port = %d dbname = %s user = %s password = %s connect_timeout = 60",
        sDbHost,iDbPort,sDbName,sDbUser,sDbPass);
    pgsqldbConn = PQconnectdb(connectInfo);
    if(!pgsqldbConn)
    {
        return 0;
    }
    else if(PQstatus(pgsqldbConn) != CONNECTION_OK)
    {
        return 0;
    }
#endif

    return 1;
}

const char * db_error()
{
#ifdef HAVE_MYSQL_CLIENT
    return mysql_error(&mysqldbConn);
#endif

#ifdef HAVE_PGSQL_CLIENT
    return PQerrorMessage(pgsqldbConn);
#endif
    return NULL;
}
