#ifndef WIN32
#include <dlfcn.h>
#endif
#include "config.hpp"
#include "log.hpp"
#include "sql_api.hpp"
#include "misc.hpp"

// dlopen handler
static void *lib_handle = NULL;

static int (*p_db_col_int)(db_struct *, int) = NULL;
static long long (*p_db_col_long_long)(db_struct *, int) = NULL;
static char * (*p_db_col_text)(db_struct *,int) = NULL;
static char * (*p_db_escape_string)(const char *, unsigned int) = NULL;
static int (*p_db_fetch_row)(db_struct *) = NULL;
static int (*p_db_query)(db_struct *,const char *,int) = NULL;
static int (*p_db_exec)(const char *) = NULL;
static int (*p_db_changes)() = NULL;
static void (*p_db_cleanup)(db_struct *) = NULL;
static int (*p_db_close)() = NULL;
static const char* (*p_db_error)() = NULL;

int (*p_db_load)(const char *sDbHost, const char *sDbUser, const char *sDbPass, const char *sDbName, int iDbPort) = NULL;

#ifdef __cplusplus
extern "C" {
#endif

int db_init(const std::string& type)
{
#ifdef WIN32
	return 1;
#else
  if (type == "mysql") {
    lib_handle = dlopen("/usr/lib/libgsql-mysql.so", RTLD_LAZY);
    if (!lib_handle) 
    {
      logevent(ERR, "Failed to load library libmysql_api.so: %s\n", dlerror());
      return 0;
    }    
  } else if (type == "pgsql") {
    lib_handle = dlopen("/usr/lib/libgsql-pgsql.so", RTLD_LAZY);
    if (!lib_handle)
    {
      logevent(ERR, "Failed to load library libpgsql_api.so: %s\n", dlerror());
      return 0;
    }
  }

  p_db_load = (int (*)(const char*, const char*, const char*, const char*, int))dlsym(lib_handle, "db_load");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_load: %s\n", dlerror());
    return 0;
  }

  p_db_exec = (int (*)(const char*))dlsym(lib_handle, "db_exec");
  if ((dlerror()) != NULL)  
  {
    logevent(ERR, "failed to load function db_exec: %s\n", dlerror());
    return 0;
  }

  p_db_col_int = (int (*)(db_struct*, int))dlsym(lib_handle, "db_col_int");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_col_int: %s\n", dlerror());
    return 0;
  }

  p_db_col_long_long = (long long int (*)(db_struct*, int))dlsym(lib_handle, "db_col_long_long");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_col_long_long: %s\n", dlerror());
    return 0;
  }

  p_db_col_text = (char* (*)(db_struct*, int))dlsym(lib_handle, "db_col_text");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_col_text: %s\n", dlerror());
    return 0;
  }

  p_db_fetch_row = (int (*)(db_struct*))dlsym(lib_handle, "db_fetch_row");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_fetch_row: %s\n", dlerror());
    return 0;
  }

  p_db_escape_string = (char* (*)(const char*, unsigned int))dlsym(lib_handle, "db_escape_string");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_escape_string: %s\n", dlerror());
    return 0;
  }

  p_db_query = (int (*)(db_struct*, const char*, int))dlsym(lib_handle, "db_query");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_query: %s\n", dlerror());
    return 0;
  }

  p_db_changes = (int (*)())dlsym(lib_handle,"db_changes");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_changes: %s\n", dlerror());
    return 0;
  }

  p_db_cleanup = (void (*)(db_struct*))dlsym(lib_handle, "db_cleanup");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_cleanup: %s\n", dlerror());
    return 0;
  }

  p_db_close = (int (*)())dlsym(lib_handle, "db_close");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_close: %s\n", dlerror());
    return 0;
  }

  p_db_error = (const char* (*)())dlsym(lib_handle, "db_error");
  if ((dlerror()) != NULL)
  {
    logevent(ERR, "failed to load function db_error: %s\n", dlerror());
    return 0;
  }
#endif
  return 1;
}

int db_col_int(db_struct *db, int col)
{
  if (p_db_col_int == NULL)
    return 0;
  return (*p_db_col_int)(db, col);
}

long long db_col_long_long(db_struct *db, int col) {
  if (p_db_col_long_long == NULL)
    return 0;
  return (*p_db_col_long_long)(db,col);
}

char *db_col_text(db_struct *db, int col) {
  if (p_db_col_text == NULL)
    return NULL;
  return (*p_db_col_text)(db,col);
}

int db_fetch_row(db_struct *db) {
  if (p_db_fetch_row == NULL)
    return 0;
  return (*p_db_fetch_row)(db);
}

char *db_escape_string(const char *str,unsigned int length) {
  if (p_db_escape_string == NULL)
    return NULL;
  return (*p_db_escape_string)(str,length);
}

int db_query(db_struct *db, const char *q,int size) {
  if (p_db_query == NULL)
    return 0;
  return (*p_db_query)(db,q,size);
}

int db_exec(const char *q) {
  if (p_db_exec == NULL)
    return 0;
  return (*p_db_exec)(q);
}

int db_changes() {
  if (p_db_changes == NULL) 
    return 0;
  return (*p_db_changes)();
}

void db_cleanup(db_struct *db) {
  if (p_db_cleanup == NULL)
    return;
  (*p_db_cleanup)(db);
}

int db_close() {
  if (p_db_close == NULL)
    return 0;
#ifndef WIN32
  (*p_db_close)();
  dlclose(lib_handle);
#endif
  return 1;
}

int db_load(const char *sDbHost, const char *sDbUser, const char *sDbPass, const char *sDbName, int iDbPort) {
  if (p_db_load == NULL)
    return 0;
  return (*p_db_load)(sDbHost,sDbUser,sDbPass,sDbName,iDbPort);
}

const char * db_error()
{
  if (p_db_error == NULL)
    return NULL;
  return (*p_db_error)();
}

#ifdef __cplusplus
}
#endif

