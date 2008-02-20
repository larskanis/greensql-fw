FILES:= sql.o sql.tab.o parser.o

CPP=g++
CFLAGS:=-Wall -DPARSER_DEBUG -g
#ifndef CXXFLAGS
CXXFLAGS:=-DPARSER_DEBUG -g -Wno-deprecated -Wall
#endif

objs:	$(FILES)

parser: $(FILES)
	$(CXX) $(CXXFLAGS) -o parser $(FILES)

sql.cpp:
	flex -i -osql.cpp sql.lex
	bison -d -v sql.ypp

sql.tab.cpp:
	

clean:
	rm -rf parser sql.tab.hpp sql.output sql.tab.cpp sql.cpp *.o
