//
// GreenSQL event logging functions.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "log.hpp"
#include "config.hpp"
#include <stdarg.h> //for va_list 
#include <stdio.h>  //for STDOUT
#include <string.h> // for memset
#include <ctype.h> // for isascii

// for fstat
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif

static bool log_reload();
static void printline(const unsigned char * data, int max);
static FILE * log_file = stdout;
static int log_level = 3;

bool log_init(std::string & file, int level)
{
    log_level = level;

    FILE * fp = fopen(file.c_str(), "a+");

    if (fp == NULL)
    {
        return false;
    }
    log_file = fp;
    return true;
}

bool log_close()
{
    if (log_file != stdout)
    {
        fclose(log_file);
    }
    log_file = stdout;
    return true;
}

static bool log_reload()
{
    return true;

    GreenSQLConfig * cfg = GreenSQLConfig::getInstance();
    struct stat f_stat;

    if (stat(cfg->log_file.c_str(), &f_stat) == 0)
        return true;

    // log file was not found. reload it
    log_close();
    log_init(cfg->log_file, cfg->log_level);
    return true; 
}

/*
 * This is a log variadic function
 *
 */
void logevent(ErrorType type, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char * error;

    if (log_level < (int) type)
    {
      va_end(ap);
      return;
    }
 
    switch (type)
    {
        case CRIT:      error = "CRIT:      ";
	                break;
	case ERR:     error = "ERROR:     ";
			break;
	case INFO:      error = "INFO:      ";
	                break;
        case DEBUG:     error = "DEBUG:     ";
                        break;
        case NET_DEBUG: error = "NET_DEBUG: ";
                        break;
	case SQL_DEBUG: error = "SQL_DEBUG: ";
			break;
	case STORAGE:   error = "STORAGE:   ";
			break;
        default:        error = "UNKNOWN:   ";
                        break;
    }
    
    log_reload();
    fprintf(log_file, error);

    vfprintf(log_file, fmt, ap);
    va_end(ap);
    fflush(log_file);
}


void loghex(ErrorType type, const unsigned char * data, int size)
{
    char * error;

    if (size == 0)
        return;
    if (log_level < (int) type)
	    return;
    
    switch (type)
    {
        case CRIT:      error = "CRIT:      ";
                        break;
        case ERR:       error = "ERROR:     ";
                        break;
        case INFO:      error = "INFO:      ";
                        break;
        case DEBUG:     error = "DEBUG:     ";
                        break;
        case NET_DEBUG: error = "NET_DEBUG: ";
                        break;
        case SQL_DEBUG: error = "SQL_DEBUG: ";
                        break;
        case STORAGE:   error = "STORAGE:   ";
                        break;
        default:        error = "UNKNOWN:   ";
                        break;
    }

    int lines = size / 16;
    int i = 0;
    
    log_reload();

    for (i = 0; i < lines; i++)
    {
       fprintf(log_file, error);
       printline(data+i*16, 16);
    }
    // ord(size%16)
    int ord = (((unsigned char)(size<<4)) >>4);
    if ( ord > 0)
    {
        fprintf(log_file, error);
        printline(data+i*16, ord);
    }
    fflush(log_file);
}

static void printline(const unsigned char * data, int max)
{

    int j = 0;
    char temp[256];
    memset(temp, ' ', sizeof(temp));
    temp[sizeof(temp)-1] = 0;
    unsigned char b;

    for(j = 0; j < max; j++)
    {
       b = data[j];
       if (isalnum(b) || b == ' ' || ispunct(b))
           temp[j] = data[j];
       else
           temp[j] = '.';
    }
    
    // print hex
    temp[18] = '|';

    for (j = 0; j < max; j++)
    {
       b = (data[j]>>4);
       b += ( b > 9) ? 'A'-10 : '0';
       temp[20+j*3] = b;

       b = ((unsigned char)(data[j]<<4)) >>4;
       b += ( b > 9) ? 'A'-10 : '0';
       temp[20+j*3+1] = b;

   }
   temp[20+j*3] = '\n';
   temp[20+j*3+1] = 0;

   fprintf(log_file, "%s", temp);

}
