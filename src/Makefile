
OBJS:=main.o buffer.o greensql.o config.o connection.o normalization.o log.o proxymap.o dbmap.o patterns.o misc.o riskengine.o dbpermobj.o alert.o mysql/mysql_con.o

CPP=g++

CFLAGS:=-Wall -D_REENTRANT -O2
CXXFLAGS:=-g -Wno-deprecated -Wall -I/usr/include/mysql/
LIBS:=-L/usr/local/lib -lmysqlclient -levent -lpcre

greensql-fw: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS) -o greensql-fw $(OBJS)

clean:
	rm greensql-fw *.o -rf
