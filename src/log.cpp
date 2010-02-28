//
// GreenSQL event logging functions.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "log.hpp"
#include <time.h>
#include <string.h>
//#include <syslog.h>
#include "config.hpp"
#include <stdarg.h> //for va_list 
#include <stdio.h>  //for STDOUT
#include <ctype.h> // for isascii

// for fstat
#include <sys/types.h>
#include <sys/stat.h>

static bool log_reload();
static void printline(const unsigned char * data, int max);
static FILE * log_file = stdout;
static int log_level = 3;
static char month_str[][4] = { {"Jan"}, {"Feb"}, {"Mar"}, {"Apr"}, {"May"}, {"Jun"}, {"Jul"}, {"Aug"}, {"Sep"}, {"Oct"}, {"Nov"}, {"Dec"}, {NULL} };

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
    GreenSQLConfig * cfg = GreenSQLConfig::getInstance();
    struct stat f_stat;
  
    if (log_file != stdout) 
    {
      // check if file was deleted 
      if ( fstat(fileno(log_file), &f_stat) == 0 )
      {
        // check number of har links, 0 - deleted
        if (f_stat.st_nlink != 0)
          return true; 
      }
    }
    //if (stat(cfg->log_file.c_str(), &f_stat) == 0)
    //  return true;
    
    
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
    //va_start(ap, fmt);
    const char * error;
    //int facility;
    struct tm *now;
    time_t tval;
    
    if (log_level < (int) type)
    {
      va_end(ap);
      return;
    }

    va_start(ap, fmt);
    tval = time(NULL);
    now = localtime(&tval);
        
    

    switch (type)
    {
      case CRIT:
        error = "CRIT      ";
        //facility = LOG_CRIT;
        break;
      case ERR:
        error = "ERROR       ";
        //facility = LOG_ERR;
        break;
      case INFO:
        error = "INFO      ";
        //facility = LOG_INFO;
        break;
      case DEBUG:
        error = "DEBUG     ";
        //facility = LOG_DEBUG;
        break;
      case NET_DEBUG:
        error = "NET_DEBUG ";
        //facility = LOG_NOTICE;
        break;
      case SQL_DEBUG:
        error = "SQL_DEBUG ";
        //facility = LOG_NOTICE;
         break;
      case STORAGE:
        error = "STORAGE   ";
        //facility = LOG_NOTICE;
        break;
      default:
        error = "UNKNOWN   ";
        //facility = LOG_NOTICE;
        break;
    }
    
    log_reload();
    fprintf(log_file,"[%02d/%s/%02d %d:%02d:%02d] %s",now->tm_mday, month_str[now->tm_mon], now->tm_year+1900, now->tm_hour, now->tm_min, now->tm_sec, error );

    // vsyslog can be used instead
    //
    //char syslog_buffer[512];
    //vsprintf(syslog_buffer,fmt,ap);
    //syslog(facility,syslog_buffer);

    vfprintf(log_file, fmt, ap );
    va_end(ap);
    fflush(log_file);
}


void loghex(ErrorType type, const unsigned char * data, int size)
{
    const char * error;
    struct tm *now;
    time_t tval;

    if (size == 0)
        return;
    if (log_level < (int) type)
        return;
   
    tval = time(NULL);
    now = localtime(&tval);
 
    switch (type)
    {
      case CRIT:
        error = "CRIT      ";
        break;
      case ERR:
        error = "ERROR     ";
        break;
      case INFO:
        error = "INFO      ";
        break;
      case DEBUG:
        error = "DEBUG     ";
        break;
      case NET_DEBUG:
        error = "NET_DEBUG ";
        break;
      case SQL_DEBUG:
        error = "SQL_DEBUG ";
        break;
      case STORAGE:
        error = "STORAGE   ";
        break;
      default:
        error = "UNKNOWN   ";
        break;
    }

    int lines = size / 16;
    int i = 0;
    
    log_reload();

    for (i = 0; i < lines; i++)
    {
      /*fprintf(log_file, error);*/
      fprintf(log_file,"[%02d/%s/%02d %d:%02d:%02d] %s", now->tm_mday, month_str[now->tm_mon], now->tm_year+1900,now->tm_hour, now->tm_min, now->tm_sec, error );
      printline(data+i*16, 16);
    }
    // ord(size%16)
    int ord = (((unsigned char)(size<<4)) >>4);
    if ( ord > 0)
    {
      /*fprintf(log_file, error);*/
      fprintf(log_file,"[%02d/%s/%02d %d:%02d:%02d] %s", now->tm_mday, month_str[now->tm_mon], now->tm_year+1900, now->tm_hour, now->tm_min, now->tm_sec, error );
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
