//
// GreenSQL MySQL Connection class code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "mysql_con.hpp"

#include "../normalization.hpp"
#include "../dbmap.hpp"

MySQLConnection::MySQLConnection(int id): Connection(id)
{
    first_request = true;
    longResponse = false;
    longResponseData = false;
    lastCommandId = (MySQLType)0;
}

MySQLConnection::~MySQLConnection()
{
}

bool MySQLConnection::parseRequest(std::string & request, bool & hasResponse)
{
    std::string query;

    int size = request_in.size();
    if (size < 3)
        return true;

    const unsigned char * data = request_in.raw();
    int request_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
        
    //check if we got full packet
    if (request_size > size)
    {
        // waite for more data
        return true;
    }
    
    if (first_request == true)
    {
	loghex(SQL_DEBUG, data, request_size);
	unsigned int type = (data[7] << 24 | data[6]<<16 | 
			data[5] << 8 | data[4]);
	if (size < 10)
	    return false;
	
	if (type & 512)
	    logevent(SQL_DEBUG, "PROTOCOL4.1\n");
	if (type & 1)
            logevent(SQL_DEBUG, "LONG PWD\n");

        if ((type & 512) && size > 36)
	{
            logevent(SQL_DEBUG, "USERNAME: %s\n", data+36);
	    if (strlen((const char *)data+36) > 0)
	    {
                db_user = (const char *)data+36;
	    }
	    int temp = 36 + strlen( (const char*)data+36);
	    //check if we have space for dnbame and pwd
	    if (temp+1 > size)
                return false;
	    int temp2 = data[temp+1];
	    if (temp2 == 0)
	    {
                logevent(SQL_DEBUG, "DATABASE: %s\n", data+temp+1);
		db_name = (const char *)data+temp+1;
                db = dbmap_find(iProxyId, db_name);
	    }
	    else if (temp+temp2+2 < size)
	    {
		logevent(SQL_DEBUG, "DATABASE: %s\n", data+temp+temp2+2);
		db_name = (const char *)data+temp+temp2+2;
		db = dbmap_find(iProxyId, db_name);
		
	    }
	}
	else if (size > 10)
	{
            logevent(SQL_DEBUG, "USERNAME: %s\n", data+9);
            if (strlen((const char *)data+9) > 0)
	    {
                db_user = (const char*)data+9;
	    }
	    
            int temp= 9 + strlen((const char*)data+9) + 1;
	    if (temp > size)
	    {
		request_in.pop(request, request_size);
		first_request = false;
                return false;
	    }
            while(temp < size && data[temp] != '\0')
                temp++;
            if (temp == size)
	    {
		request_in.pop(request, request_size);
		first_request = false;
                return true;
	    }
            temp++;
	    if (temp == size)
	    {
		request_in.pop(request, request_size);
		first_request = false;
                return true;
	    }
            int end = temp;
	    while (end < size && data[end] != '\0')
                end++;
	    db_name = "";
	    db_name.append((const char*)data+temp, end-temp);
            logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());
	}
        first_request = false;
    }

    lastCommandId = (MySQLType)(int)(data[4]);
    if (data[4] == MYSQL_QUERY ||
        data[4] == MYSQL_FIELDLIST ||
        //data[4] == MYSQL_STATISTICS ||
        data[4] == MYSQL_PROCESSINFO)
    {
        //logevent(SQL_DEBUG, "Long SQL Request - %d\n", data[4]);
        longResponse = true;
        longResponseData = false;
    }

    request_in.pop(request, request_size);
    
    // we got command?
    if (data[4] == MYSQL_QUIT) {
        logevent(SQL_DEBUG, "QUIT command :\n");
    } else if (data[4] == MYSQL_QUERY) {
        query = (char *)data+5;
        logevent(SQL_DEBUG, "QUERY command[%s]: %s\n", 
			db_name.c_str(), data+5);
        // query must not be empty
	// otherwise system can crash
	if (query[0] != '\0')
          if ( check_query(query) == false)
	{
            // bad query - block it
	    request = ""; // do not send it to backend server
	    std::string response = "";
            blockResponse(response);
	    response_in.append(response.c_str(), response.size());
	    hasResponse = true;
	} else {
	    hasResponse = false;
	}

    } else if (data[4] == MYSQL_PREPARE) {
        logevent(SQL_DEBUG, "PREPARE QUERY: %s\n", data+5);
    } else if (data[4] == MYSQL_EXEC) {
        logevent(SQL_DEBUG, "EXECUTE QUERY: %s\n", data+5);
    } else if (data[4] == MYSQL_DB) {
        logevent(SQL_DEBUG, "DATABASE: %s\n", data+5);
	db_new_name = (const char *)data+5;
    } else {
	logevent(SQL_DEBUG, "UNKNOWN COMMAND\n");
	loghex(SQL_DEBUG, data, request_size);   
    }

    return true;
}

bool MySQLConnection::parseResponse(std::string & response)
{
    unsigned int size = (unsigned int) response_in.size();
    unsigned int header_size = 0;
    unsigned int full_size = 0;
    int n_fields = 0;
    unsigned int temp = 0;

    if (size < 3)
    {
       logevent(NET_DEBUG, "received %d bytes of response\n", size);
       return false;
    }
    const unsigned char * data = response_in.raw();
    unsigned int response_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;

    logevent(NET_DEBUG, "packet size expected %d bytes (received %d)\n", 
    	    response_size, size);

    if (response_size > size )
        return false;

    if (data[4] == 0xff && response_size > 7)
    {
        logevent(SQL_DEBUG, "SQL_ERROR: %s\n", data+7);
        response_in.pop(response, response_size);
        longResponse = false;
        longResponseData = false;
        return true;
    } 
    // check if the previos command was change db
    if (lastCommandId == MYSQL_DB)
    {
        db_name = db_new_name;

        //load new db settings
        db = dbmap_find(iProxyId, db_name);
    }
    if (longResponse == false && 
        longResponseData == false)
    {
        response_in.pop(response, response_size);
        return true;
    }

    header_size = response_size;
   
    if (longResponse == true)
    {
        // first packet contains number of fields and 
        // possible number of records
        n_fields = data[4];

        logevent(NET_DEBUG, "Long response. Number of fields %d\n",
                n_fields);

        // could happen when using "SET command"
        if (n_fields == 0)
        {
            //loghex(SQL_DEBUG, data, response_size);
            // just return
            response_in.pop(response, response_size);
            longResponse = false;
            longResponseData = false;
            return true;
        }
        temp = 0;

        if (header_size + temp + 5 < size &&
            data[header_size+temp+4] != MYSQL_ENDROW)
        {
            //add size of the last packet
            //header_size += temp;

            //sometime it sends more data in the header
            while (header_size + temp < size - 5 &&
                   data[header_size + temp+4] != MYSQL_ENDROW)
            {
                header_size += temp;
                temp = (data[header_size+2] <<16 |
                        data[header_size+1] << 8 |
                        data[header_size+0]) + 4;
                logevent(NET_DEBUG, "Additionla Header size %d, type %d.\n",
                         temp, data[header_size + 4]);
            }
        }
        // check if we have space left for the end of the header packet
        if (header_size + temp + 5 > size)
        {
            // read more data to decide
            logevent(NET_DEBUG, "Read more data to decide. (%d-%d)\n", 
			    header_size + temp + 5, size);
            return false;
        }
        // now must go end of fields packet
        if (data[header_size+temp+4] == MYSQL_ENDROW)
        {
            // add size of the last packet
            header_size += temp;

            logevent(NET_DEBUG, "End of fields packet %d-%d\n",
    	    header_size, size);

            temp = (data[header_size+2] <<16 |
                    data[header_size+1] << 8 |
                    data[header_size+0]) + 4;

            logevent(NET_DEBUG, "Last Field size %d.\n", temp);
        } else {
            logevent(NET_DEBUG, "Packet end/type missmatch %d.\n", 
                     data[header_size+temp+4]);
            return false;
        }
        if (header_size + temp <= size)
        {
            header_size += temp;
            if (lastCommandId != MYSQL_FIELDLIST)
                longResponseData = true;
            longResponse = false;
        
            logevent(NET_DEBUG, "End of header-start of data (left %d bytes).\n", size - header_size);
            full_size = header_size;
        } else {
            logevent(NET_DEBUG, "Failed to get end of packet\n");
            return false;
        }
    }

    if (longResponseData == true && full_size <= size)
    {
        logevent(NET_DEBUG, "Data packet, size left %d\n", size - full_size);

        temp = (data[full_size+2] <<16 |
                data[full_size+1] << 8 |
                data[full_size+0]) + 4;
    
        while ((data[full_size+4] != MYSQL_ENDROW) &&
           (full_size+temp+5 <= size))
        {
            // data packet
            full_size += temp;
            temp = (data[full_size+2] <<16 |
                    data[full_size+1] << 8 |
                    data[full_size+0]) + 4;
        }

        if (data[full_size+4] == MYSQL_ENDROW &&
            full_size+temp <= size)
        {
            full_size += temp;
            longResponseData = false;
            logevent(NET_DEBUG, "End of long response.\n");
        }
    }
    
    response_in.pop(response, full_size);
    
    //debug - block all responses
    //response = "";
    //blockResponse(response); 

    return true;
}

bool MySQLConnection::blockResponse(std::string & response)
{
    // we will return an empty result
    //char data[] = {1,0,0,1,0};
    //char data[] = {05, 00, 00, 01, 00, 00, 00, 02, 00};
    char data[] = {7,0,0,1, 0,0,0, 2,0, 0,0 }; // OK response
    
    response.append(data, 11);
    return true;
}
