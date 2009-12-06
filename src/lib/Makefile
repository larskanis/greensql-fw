CXXFLAGS:=-g -Wall -I/usr/local/include/
MYSQL_INC:=-I/usr/include/mysql/ -I/usr/local/include/mysql/
MYSQL_LIBS:=-L/usr/lib64/mysql -L/usr/local/lib/mysql -L/usr/lib/mysql -lmysqlclient
PGSQL_INC:=-I/usr/include/postgresql -I/usr/include/pgsql/
PGSQL_LIBS:=-lpq

api:
	cc $(CXXFLAGS) $(MYSQL_INC) -DHAVE_MYSQL_CLIENT -fPIC -c -Wall sql_api.c
	cc $(CXXFLAGS) $(MYSQL_LIBS) -shared -Wl,-soname,libgsql-mysql.so.1  -DHAVE_MYSQL_CLIENT -o libgsql-mysql.so.1 sql_api.o
	cc $(CXXFLAGS) $(PGSQL_INC) -DHAVE_PGSQL_CLIENT -fPIC -c -Wall sql_api.c
	cc $(CXXFLAGS) $(PGSQL_LIBS) -shared -Wl,-soname,libgsql-pgsql.so.1  -DHAVE_PGSQL_CLIENT -o libgsql-pgsql.so.1 sql_api.o

clean:
	rm -rf *.so.1
