
OBJS:=main.o buffer.o greensql.o config.o connection.o normalization.o log.o proxymap.o dbmap.o patterns.o misc.o riskengine.o dbpermobj.o alert.o mysql/mysql_con.o

CPP=g++

CFLAGS:=-Wall -D_REENTRANT -O2
CXXFLAGS:=-g -Wno-deprecated -Wall -I/usr/include/mysql/ -I/usr/local/include/ -I/usr/local/include/mysql/
LIBS:=-L/usr/local/lib -L/usr/local/lib/mysql -L/usr/lib/mysql -lmysqlclient -levent -lpcre

greensql-fw: $(OBJS)
	cd parser; make CXXFLAGS="$(CXXFLAGS)"
	$(CXX) $(CXXFLAGS) $(LIBS) -o greensql-fw $(OBJS) parser/parser.o parser/sql.o parser/sql.tab.o

mysql/mysql_con.o:
	$(CXX) $(CXXFLAGS) -c mysql/mysql_con.cpp -o mysql/mysql_con.o

clean:
	rm -rf greensql-fw *.o mysql/*.o
	cd parser; make clean; cd ..
