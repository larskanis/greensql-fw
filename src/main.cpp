//
// GreenSQL main function.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include <iostream>

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h> 
#include <tchar.h>
#include <crtdbg.h>
#else
#include <signal.h>
#endif

#include <event.h>

#include "config.hpp"
#include "greensql.hpp"
#include "mysql/mysql_con.hpp"
#include "proxymap.hpp"
#include "dbmap.hpp"


#if WIN32
int initWin();
#else
int initLinux();
#endif
void clb_timeout(int fd, short which, void * arg);
void Killer(int);

#ifdef WIN32
_CrtMemState s1, s2, s3;
#endif

//GreenSQLConfig * cfg = NULL;

void Killer(int)
{
    logevent(CRIT, "Killer\n");
    GreenSQLConfig * cfg = GreenSQLConfig::getInstance();
    cfg->bRunning = false;
}

#ifdef WIN32
BOOL Hook( DWORD dwType ) 
{
    switch( dwType ) 
    {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        	Killer(1);
        	return TRUE;
        default:
        	return FALSE;
    } 
} 
#endif


#if WIN32
int _tmain(int argc, char * argv[])
#else
int main(int argc, char *argv[])
#endif
{
    try
    {
#ifdef WIN32
        SetConsoleCtrlHandler( (PHANDLER_ROUTINE) Hook, TRUE );
        _CrtMemCheckpoint(&s1);
#endif
	GreenSQLConfig * cfg = GreenSQLConfig::getInstance();
        std::string conf_path = "./";

	if (argc > 2)
	{
            if (strcmp(argv[1], "-p") == 0)
	    {
                conf_path = argv[2];
		logevent(DEBUG, "Config file path: %s\n", 
				conf_path.c_str());
	    } else {
               fprintf(stderr, "Unrecognized parameter\n\n");
	       fprintf(stderr, "Usage: %s -p DIRECTORY\n\n", argv[0]); 
	       fprintf(stderr, "DIRECTORY is a location of the config files\n");
	       return -1;
	    }
	    
	} else if (argc > 1)
	{
            fprintf(stderr, "Unrecognized parameter\n\n");
            fprintf(stderr, "Usage: %s -p DIRECTORY\n\n", argv[0]);
	    fprintf(stderr, "DIRECTORY is a location of the config files\n");
            return -1;
	}
	
	if (cfg->load(conf_path) == false)
	{
            fprintf(stderr, "Failed to load config file: %s/greensql.conf\n\n", 
			    conf_path.c_str());
	    fprintf(stderr, "Specify location of the conf. file using \"-p\" parameter.\n\n");
	    fprintf(stderr, "Usage: %s -p DIRECTORY\n\n", argv[0]);
	    fprintf(stderr, "DIRECTORY is a location of the config files\n");
	    return -1;
	}
        if (log_init(cfg->log_file, cfg->log_level) == false)
        {
            fprintf(stderr, "Failed to open log file %s\n",
                            cfg->log_file.c_str());
            return -1;
        }
	logevent(INFO, "Application started\n");

        if (cfg->load_db() == false)
	{
            fprintf(stderr, "Failed to connect to db storage.\n");
	    return -1;
	}
        if (cfg->load_patterns(conf_path) == false)
        {
            fprintf(stderr, "Failed to load list of patterns.\n");
	    return -1;
        }
#if WIN32
        initWin();
#else
        initLinux();
#endif

        event_init();

        struct event tEvent;
        memset(&tEvent,0, sizeof(struct event));
        struct timeval delay;
        delay.tv_sec=0;
        delay.tv_usec=100;
        evtimer_set(&tEvent, clb_timeout, &tEvent);
        evtimer_add(&tEvent, &delay);
       
        proxymap_init();
        dbmap_init();
		
        event_loop(0);
	logevent(DEBUG, "end of the event loop\n");
	
//the followint function must clear memory used by event system
#ifdef event_base_free
	event_base_free(NULL);
#endif

        proxymap_close();
	dbmap_close();
        log_close(); 
	GreenSQLConfig::free();

#ifdef WIN32
        _CrtMemCheckpoint(&s2);
        if(_CrtMemDifference(&s3,&s1, &s2))
       	    _CrtMemDumpStatistics(&s3);
        _CrtDumpMemoryLeaks();
        Beep( 440, 300 );
#endif
	std::cout << "quit:" << std::endl; 
        
    }
    catch(char * str)
    {
	    std::cout << "Main exception:" << str << std::endl; 
    }
    exit(0);
}

#if WIN32
int initWin()
{
    WSADATA wsaData;
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
    perror("WSAStartup");

    return 1;
}
#else

int initLinux()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGQUIT, Killer);
    signal(SIGHUP, Killer);
    signal(SIGINT, Killer);
    signal(SIGTERM, Killer);
    return 1;
}
#endif
static int counter = 0;

void clb_timeout(int fd, short which, void * arg)
{
    struct timeval delay;
    delay.tv_sec=0;
    delay.tv_usec=100;

    struct event * tEvent = (struct event*) arg;

    event_del(tEvent);

    GreenSQLConfig * cfg = GreenSQLConfig::getInstance();

    if (cfg->bRunning == false)
    {
        proxymap_close();
    } else {
        evtimer_set(tEvent, clb_timeout, tEvent);
        evtimer_add(tEvent, &delay);
	counter++;
	if (counter == 1000)
	{
            //logevent(INFO, "Timer fired 1000 times\n");
	    counter = 0;
	    proxymap_reload();
	    dbmap_reload();
	}
    }
}
