
#ifndef GREEN_SQL_API_HPP
#define GREEN_SQL_API_HPP

int db_query(db_struct *db,const char *q,int size);
int db_exec(const char *q);
int db_fetch_row(db_struct *db);
int db_col_int(db_struct *db, int col);
long long db_col_long_long(db_struct *db, int col);
const char *db_col_text(db_struct *db, int col);
int db_changes();
char *db_escape_string(const char *str, unsigned int length);
void db_cleanup(db_struct *db);
int db_load(const char *sDbHost, const char *sDbUser, const char *sDbPass, const char *sDbName, int iDbPort);
int db_close();
const char * db_error();

#endif
