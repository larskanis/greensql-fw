//
// GreenSQL main function.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

//#include <iostream>

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h> 
#include <tchar.h>
#include <crtdbg.h>
#else
#include <signal.h>
#include <sys/types.h>
#endif

#include <event.h>

#include "config.hpp"
#include "greensql.hpp"
#include "mysql/mysql_con.hpp"
#include "pgsql/pgsql_con.hpp"
#include "proxymap.hpp"
#include "dbmap.hpp"
#include "alert.hpp"

static bool fix_dir_name(std::string & conf_dir);

#if WIN32
int initWin();
#else
int initLinux();
#endif
void clb_timeout(int fd, short which, void * arg);
void Killer(int);

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
    //try
    {
#ifdef WIN32
        SetConsoleCtrlHandler( (PHANDLER_ROUTINE) Hook, TRUE );
#endif
	GreenSQLConfig * cfg = GreenSQLConfig::getInstance();
        std::string conf_path = "";
#ifndef WIN32
	conf_path = "./";
#endif
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

        fix_dir_name(conf_path);	
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
        if (mysql_patterns_init(conf_path) == false)
        {
            fprintf(stderr, "Failed to load MySQL list of patterns.\n");
            return -1;
        }
		if (pgsql_patterns_init(conf_path) == false)
		{
			fprintf(stderr, "Failed to load PGSQL list of patterns.\n");
			return -1;
		}
#if WIN32
        initWin();
#else
        initLinux();
        event_init();
#endif

        struct event tEvent;
        memset(&tEvent,0, sizeof(struct event));
        struct timeval delay;
        delay.tv_sec=1;
        delay.tv_usec=0;
#ifndef WIN32
        evtimer_set(&tEvent, clb_timeout, &tEvent);
        evtimer_add(&tEvent, &delay);
#endif
       
        if (proxymap_init() == false)
        {
           logevent(DEBUG, "Failed to open all server sockets, closing application\n");
           exit(0);
        }
        dbmap_init();
	agroupmap_init();
#ifndef WIN32	
        event_loop(0);
#endif
        //logevent(DEBUG, "end of the event loop\n");
	
//the followint function must clear memory used by event system
#ifdef event_base_free
	event_base_free(NULL);
#endif

        proxymap_close();
	dbmap_close();
        log_close(); 
	GreenSQLConfig::free();

// #ifdef WIN32
//         _CrtMemCheckpoint(&s2);
//         if(_CrtMemDifference(&s3,&s1, &s2))
//        	    _CrtMemDumpStatistics(&s3);
//         _CrtDumpMemoryLeaks();
//         Beep( 440, 300 );
// #endif
	//std::cout << "quit:" << std::endl; 
        
    }
/*
    catch(char * str)
    {
	    std::cout << "Main exception:" << str << std::endl; 
    }
*/
    exit(0);
}

#if WIN32
int initWin()
{
    WSADATA wsaData;
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        // failed to init windows networking.
    }

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
    delay.tv_sec=1;
    delay.tv_usec=0;

    struct event * tEvent = (struct event*) arg;
#ifndef WIN32
    event_del(tEvent);
#endif

    GreenSQLConfig * cfg = GreenSQLConfig::getInstance();

    //logevent(INFO, "timer fired\n");
    if (cfg->bRunning == false)
    {
        proxymap_close();
    } else {
#ifndef WIN32
        evtimer_set(tEvent, clb_timeout, tEvent);
        evtimer_add(tEvent, &delay);
#endif
	counter++;
	if (counter == 5)
	{
            //logevent(INFO, "Timer fired 1000 times\n");
	    counter = 0;
	    proxymap_reload();
	    dbmap_reload();
	    agroupmap_reload();
	}
    }
}

static bool fix_dir_name(std::string & conf_dir)
{
  int len = conf_dir.length();
#if WIN32
  if (conf_dir[len-1] != '\\')
  {
    conf_dir += '\\';
  }
#else
  if (conf_dir[len-1] != '/')
  {
    conf_dir += "/";
  }
#endif
  return true; 
}

