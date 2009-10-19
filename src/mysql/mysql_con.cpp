//
// GreenSQL MySQL Connection class code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "mysql_con.hpp"

#include "../normalization.hpp"
#include "../config.hpp"
#include "../dbmap.hpp"
static SQLPatterns mysql_patterns;

bool mysql_patterns_init(std::string & path)
{
    std::string file = path + "mysql.conf";

    return mysql_patterns.Load(file);
}

SQLPatterns * MySQLConnection::getSQLPatterns()
{
    return & mysql_patterns;
}

MySQLConnection::MySQLConnection(int id): Connection(id)
{
    first_request = true;
    longResponse = false;
    longResponseData = false;
    lastCommandId = (MySQLType)0;
    db_type = "mysql";
    db = dbmap_default(id, db_type.c_str());
    mysql41 = false;
}

MySQLConnection::~MySQLConnection()
{
}

bool MySQLConnection::checkBlacklist(std::string & query, std::string & reason)
{
    //GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    bool ret = false;
    bool bad = false;
    if (db->CanAlter() == false)
    {
        ret = mysql_patterns.Match( SQL_ALTER,  query );
        if (ret == true)
        {
            reason += "Detected attempt to change database/table structure.\n";
        logevent(DEBUG, "Detected attempt to change database/table structure.\n");
        bad = true;
        }
    }
    if (db->CanDrop() == false)
    {
        ret = mysql_patterns.Match( SQL_DROP,  query );
        if (ret == true)
        {
            reason += "Detected attempt to drop database/table/index.\n";
        logevent(DEBUG, "Detected attempt to drop database/table.\n");
        bad = true;
        }
    }
    if (db->CanCreate() == false)
    {
        ret = mysql_patterns.Match( SQL_CREATE,  query );
        if (ret == true)
        {
            reason += "Detected attempt to create database/table/index.\n";
        logevent(DEBUG, "Detected attempt to create database/table/index.\n");
        bad = true;
        }
    }
    if (db->CanGetInfo() == false)
    {
        ret = mysql_patterns.Match( SQL_INFO,  query );
        if (ret == true)
        {
            reason += "Detected attempt to discover db internal information.\n";
        logevent(DEBUG, "Detected attempt to discover db internal information.\n");
        bad = true;
        }
    }
    return bad;
}

bool MySQLConnection::parseRequest(std::string & request, bool & hasResponse)
{
    unsigned int full_size = request_in.size();

    if (full_size < 3)
        return true;

    std::string query = "";
    std::string response = "";

    const unsigned char * data = request_in.raw();
    unsigned int request_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
    unsigned int packet_id = data[3];
    unsigned int type = data[4];
    unsigned int max_user_len;
    unsigned int user_len;
    unsigned int db_name_len;
    unsigned int pwd_len = 0;

    logevent(SQL_DEBUG, "Client packet %d\n", packet_id);

    //check if we got full packet
    if (request_size > full_size)
    {
        // wait for more data to decide
        return true;
    }
   
    // this is a first request in the stream
    // it must contain login information. 
    if (first_request == true)
    {
        loghex(SQL_DEBUG, data, request_size);
        if (full_size < 10)
          return false;

        // client flags can be 2 or 4 bytes long
        unsigned int client_flags = (data[7] << 24 | data[6]<<16 | 
            data[5] << 8 | data[4]);
    
        if (client_flags & MYSQL_CON_PROTOCOL_V41)
        {
            logevent(SQL_DEBUG, "Client uses protocol 4.1\n");
            mysql41 = true;
        }
        if (client_flags & MYSQL_CON_LONG_PWD)
            logevent(SQL_DEBUG, "Client uses LONG PWD\n");
        if (client_flags & MYSQL_CON_COMPRESS)
            logevent(SQL_DEBUG, "Client uses traffic compression\n");
        if (client_flags & MYSQL_CON_SSL)
            logevent(SQL_DEBUG, "Client uses SSL encryption\n");

        db_name = "";
        db_user = "";

        if ((client_flags & MYSQL_CON_PROTOCOL_V41) && full_size > 36)
        {
            user_len = strlen((const char *)data+36);
            //if (user_len == 0)
            //{
            //    // read more data to decide?
            //    return false;
            //}
            if (user_len + 36 > full_size)
            {
                user_len = full_size - 37;
                db_user.append((const char*)data+36, user_len);
                logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
            } else {
                db_user = (const char *)data+36;
                logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
                //check if we have space for dnbame and pwd
                if (user_len + 36 + 1 >= full_size)
                    return false;

                int pwd_size = data[user_len+36+1];
                if (pwd_size == 0)
                {
                    db_name_len = strlen((const char*)data+user_len+36+2);
                    if (db_name_len > 0)
                    {
                        if (db_name_len > full_size-user_len-36-3)
                            db_name_len = full_size-user_len-36-3;
                        db_name.append((const char*)data+user_len+36+2, db_name_len);
                        logevent(SQL_DEBUG, "DATABASE2: %s\n", db_name.c_str());
                        db = dbmap_find(iProxyId, db_name, "mysql");
                    }
                }
                else if (user_len+36+pwd_size+2 < full_size)
                {
                    db_name_len = strlen((const char*)data+user_len+36+pwd_size+2);
                    if (db_name_len > full_size-user_len-36-pwd_size-3)
                        db_name_len = full_size-user_len-36-pwd_size-3;
                    db_name.append((const char *)data+user_len+36+pwd_size+2, db_name_len);
                    logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());
                    db = dbmap_find(iProxyId, db_name, "mysql");
                }
            }
        }
        else if (full_size > 10)
        {
            user_len = strlen((const char *)data+9);
            if (user_len > full_size - 9)
                user_len = full_size - 9;
            if (user_len != 0)
            {
                  db_user.append((const char *) data+9, user_len); 
                logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
            }

            unsigned int temp= 9 + user_len + 1;
            if (temp > full_size)
            {
                request_in.pop(request, request_size);
                first_request = false;
                return false;
            }
            while(temp < full_size && data[temp] != '\0')
                temp++;
            if (temp == full_size)
            {
                request_in.pop(request, request_size);
                first_request = false;
                return true;
            }
            temp++;
            if (temp == full_size)
            {
                request_in.pop(request, request_size);
                first_request = false;
                return true;
            }
            unsigned int end = temp;
            while (end < full_size && data[end] != '\0')
                end++;
            db_name = "";
            db_name.append((const char*)data+temp, end-temp);
            logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());
        }
        first_request = false;
        // we do not have any additional commands in first packet.
        // just send everything we got so far.
        request_in.pop(request, request_size);
        return true;
    }

    lastCommandId = (MySQLType)(int)(data[4]);
    if (type == MYSQL_QUERY ||
        type == MYSQL_FIELD_LIST ||
        //type == MYSQL_STATISTICS ||
        type == MYSQL_PROCESS_INFO ||
        type == MYSQL_STMT_FETCH)
    {
        //logevent(SQL_DEBUG, "Long SQL Request - %d\n", data[4]);
        longResponse = true;
        longResponseData = false;
    }

    // we got command?
    if (type == MYSQL_SLEEP)
    {
        logevent(SQL_DEBUG, "MYSQL SLEEP command\n");
    } else if (type == MYSQL_QUIT)
    {
        logevent(SQL_DEBUG, "MYSQL QUIT command\n");
    } else if (type == MYSQL_DB) {
        db_new_name = "";
        db_new_name.append((const char *)data+5, request_size-5);
        logevent(SQL_DEBUG, "DATABASE: %s\n", db_new_name.c_str());
    } else if (type == MYSQL_QUERY) {
        // query must not be empty
        // otherwise system can crash
        if (data[5] == '\0')
        {
            request_in.pop(request, request_size);
            return true;
        }
        unsigned int max_query_len = request_size - 5;
        query.append((char *)data+5, max_query_len);

        logevent(SQL_DEBUG, "QUERY command[%s]: %s\n", 
            db_name.c_str(), query.c_str());

        if ( check_query(query) == false)
        {
            // bad query - block it
            request = ""; // do not send it to backend server
            blockResponse(response);
            if (response_in.size() != 0)
            {
                // push it to the server response parsing queue
                response_in.append(response.c_str(), response.size());
            } else {
                // push it to the client
                response_out.append(response.c_str(), response.size());
            }
            hasResponse = true;
        } else {
            hasResponse = false;
        }
    } else if ((type == MYSQL_CREATE_DB || type == MYSQL_DROP_DB || type == MYSQL_FIELD_LIST) && 
        request_size > 5)
    {
        // we will emulate create database command and run it as if it is a regular command
        if (type == MYSQL_CREATE_DB)
            query = "create database '";
        else if (type == MYSQL_DROP_DB)
            query = "drop database '";
        else if (type == MYSQL_FIELD_LIST)
            query = "show fields from '";
        query.append((const char *)data+5, request_size-5);
        query.append("'");

        if ( check_query(query) == false)
        {
            // bad query - block it
            request = ""; // do not send it to backend server
            blockResponse(response);
            if (response_in.size() != 0)
            {
                // push it to the server response parsing queue
                response_in.append(response.c_str(), response.size());
            } else {
                // push it to the client
                response_out.append(response.c_str(), response.size());
            }
            hasResponse = true;
        } else {
            hasResponse = false;
        }    
    } else if (type == MYSQL_REFRESH)
    {
        logevent(SQL_DEBUG, "MYSQL FLUSH command\n");
    } else if (type == MYSQL_SHUTDOWN)
    {
        logevent(SQL_DEBUG, "MYSQL SHUTDOWN command\n");
    } else if (type == MYSQL_STATISTICS)
    {
        logevent(SQL_DEBUG, "MYSQL get statistics command\n");
    } else if (type == MYSQL_PROCESS_INFO)
    {
        logevent(SQL_DEBUG, "MYSQL PROCESSLIST command\n");
        query = "show processlist";
        if ( check_query(query) == false)
        {
            // bad query - block it
            request = ""; // do not send it to backend server
            blockResponse(response);
            if (response_in.size() != 0)
            {
                // push it to the server response parsing queue
                response_in.append(response.c_str(), response.size());
            } else {
                // push it to the client
                response_out.append(response.c_str(), response.size());
            }
            hasResponse = true;
        } else {
            hasResponse = false;
        }
    } else if (type == MYSQL_CONNECT)
    {
        logevent(SQL_DEBUG, "MYSQL CONNECT command\n");
    } else if (type == MYSQL_KILL && request_size >= 8)
    {
        unsigned int process_id = (data[8] << 24 | data[7]<<16 | 
            data[6] << 8 | data[5]);
        logevent(SQL_DEBUG, "MYSQL KILL %u command\n", process_id);
    } else if (type == MYSQL_DEBUG)
    {
        logevent(SQL_DEBUG, "MYSQL DEBUG command\n");
    } else if (type == MYSQL_PING)
    {
        logevent(SQL_DEBUG, "MYSQL PING command\n");
    } else if (type == MYSQL_INT_TIME)
    {
        logevent(SQL_DEBUG, "MYSQL TIME command\n");
    } else if (type == MYSQL_DELAYED_INSERT)
    {
        logevent(SQL_DEBUG, "MYSQL DELAYED INSERT command\n");
    } else if (type == MYSQL_CHANGE_USER)
    {
        max_user_len = 255;
        if (request_size - 5 < max_user_len)
            max_user_len = request_size -5;
        unsigned int user_len = strlen((const char*)data+5);
        if (user_len > max_user_len)
            user_len = max_user_len;
        db_user = "";
        db_name = "";
        db_user.append((const char*)data+5, max_user_len);
        pwd_len = 0;
        if (mysql41 == true)
        {
            pwd_len = data[5+user_len];
        } else {
            pwd_len = strlen((const char*) data+5+user_len);
        }
        if (user_len + pwd_len + 5 < full_size)
        {
            db_name_len = strlen((const char *)data+5+user_len+pwd_len);
            if (db_name_len + 5+user_len+pwd_len > full_size)
                db_name_len = full_size - 5 - user_len - pwd_len;
            db_new_name = "";
            db_new_name.append((const char*) data, db_name_len);
            if (db_name != db_new_name)
            {
                db_name = db_new_name;
                db = dbmap_find(iProxyId, db_name, "mysql");
            }
        }
        // then we have a password
        logevent(SQL_DEBUG, "MYSQL CHANGE USER command\n");
    } else if (type == MYSQL_REPL_BINLOG_DUMP)
    {
        logevent(SQL_DEBUG, "MYSQL REPL BINLOG DUMP command\n");
    } else if (type == MYSQL_REPL_TABLE_DUMP)
    {
        logevent(SQL_DEBUG, "MYSQL REPL TABLE DUMP command\n");
    } else if (type == MYSQL_REPL_CONNECT_OUT)
    {
        logevent(SQL_DEBUG, "MYSQL REPL CONNECT OUT command\n");
    } else if (type == MYSQL_REPL_REGISTER_SLAVE)
    {
        logevent(SQL_DEBUG, "MYSQL REPL REGISTER SLAVE command\n");
    } else if (type == MYSQL_STMT_PREPARE) {
        if (data[5] == '\0')
        {
            request_in.pop(request, request_size);
            return true;
        }
        unsigned int max_query_len = request_size - 5;
        query.append((const char *)data+5, max_query_len);

        logevent(SQL_DEBUG, "MYSQL PREPARE QUERY[%s]: %s\n", db_name.c_str(), query.c_str());
        //hasResponse = false;
        //return true;

        if ( check_query(query) == false)
        {
            // bad query - block it
            request = ""; // do not send it to backend server
            blockResponse(response);
            if (response_in.size() != 0)
            {
                // push it to the server response parsing queue
                response_in.append(response.c_str(), response.size());
            } else {
                // push it to the client
                response_out.append(response.c_str(), response.size());
            }
            hasResponse = true;
        } else {
            hasResponse = false;
        }

    } else if (type == MYSQL_STMT_EXEC) {
        unsigned int statement_id = (data[8] << 24 | data[7]<<16 | 
            data[6] << 8 | data[5]);
        logevent(SQL_DEBUG, "MYSQL STMT EXECUTE: %u\n", statement_id);
    } else if (type == MYSQL_LONG_DATA)
    {
        logevent(SQL_DEBUG, "MYSQL LONG DATA command\n");
    } else if (type == MYSQL_STMT_CLOSE)
    {
        unsigned int statement_id = (data[8] << 24 | data[7]<<16 | 
            data[6] << 8 | data[5]);
        logevent(SQL_DEBUG, "MYSQL STMT CLOSE: %u\n", statement_id);
    } else if (type == MYSQL_STMT_RESET)
    {
        unsigned int statement_id = (data[8] << 24 | data[7]<<16 | 
            data[6] << 8 | data[5]);
        logevent(SQL_DEBUG, "MYSQL STMT RESET: %u\n", statement_id);
    } else if (type == MYSQL_SET_OPTION)
    {
        logevent(SQL_DEBUG, "MYSQL SET OPTION command\n");
    } else if (type == MYSQL_STMT_FETCH)
    {
        unsigned int statement_id = (data[8] << 24 | data[7]<<16 | 
            data[6] << 8 | data[5]);
        logevent(SQL_DEBUG, "MYSQL STMT FETCH: %u\n", statement_id);
    } else {
        logevent(SQL_DEBUG, "UNKNOWN COMMAND\n");
        loghex(SQL_DEBUG, data, request_size);   
    }

    request_in.pop(request, request_size);
    if (hasResponse == true)
        request = "";
    //request_in.pop(request, full_size);

    return true;
}

bool MySQLConnection::parseResponse(std::string & response)
{
    unsigned int max_response_size = (unsigned int) response_in.size();
    unsigned int header_size = 0;
    unsigned int full_size = 0;
    int n_fields = 0;
    unsigned int temp = 0;
    unsigned int row_size = 0;

    if (max_response_size < 3)
    {
       logevent(NET_DEBUG, "received %d bytes of response\n", max_response_size);
       return false;
    }
    const unsigned char * data = response_in.raw();
    // max packet size is 16 MB
    unsigned int response_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
    unsigned int packet_id = data[3];
    unsigned int type = data[4];

    logevent(NET_DEBUG, "packet size expected %d bytes (received %d)\n", 
            response_size, max_response_size);
    logevent(SQL_DEBUG, "server packet %d\n", packet_id);

    //if (response_size > max_response_size )
    //{
    //    return false;
    //}

    // server sends a handshake request
    if (first_request == true && response_size > 23 && response_size <= max_response_size)
    {
        std::string db_srv_version;
        unsigned int max_srv_len = response_size - 5;
        unsigned int srv_len = strlen((const char*)data+5);
        if (srv_len > max_srv_len)
            srv_len = max_srv_len;
        db_srv_version.append((const char*)data+5, srv_len);
        if (srv_len + 19 > response_size)
        {
            // strange packet
            response_in.pop(response, max_response_size);
            return true;
        }
        // next 8 bytes - scramble data
        // next byte - zero
        unsigned int server_flags = (data[srv_len+20] << 8 | data[srv_len+19]);
        // next we have additional scramble data
        
        response_in.pop(response, response_size);
        if (server_flags & MYSQL_CON_PROTOCOL_V41)
        {
            logevent(SQL_DEBUG, "Server supports protocol 4.1\n");
        }
        if (server_flags & MYSQL_CON_SSL)
        {
            logevent(NET_DEBUG, "Disable mysql server SSL encryption capability as GreenSQL does not supports it.\n");
        }
        if (server_flags & MYSQL_CON_COMPRESS)
        {
            logevent(NET_DEBUG, "Disable mysql server traffic compression capability as GreenSQL does not supports it.\n");
        }
        // we will disable compression and SSL support
        server_flags = server_flags  & ( ~ (unsigned int)(MYSQL_CON_SSL | MYSQL_CON_COMPRESS));
        response[srv_len+19] = (server_flags & 0xff);
        response[srv_len+20] = ((server_flags >> 8) & 0xff);
        return true;
    }

    if (type == MYSQL_SRV_ERROR && response_size > 7 && response_size <= max_response_size)
    {

        unsigned int max_error_len = response_size - 7;
        unsigned int error_len = strlen((const char*)data+7);
        if (error_len > max_error_len)
            error_len = max_error_len;

        if (error_len > 0)
        {
            std::string full_error = "";
            full_error.append((const char*)data+7, error_len);

            logevent(SQL_DEBUG, "SQL_ERROR: %s\n", full_error.c_str());
        }

        response_in.pop(response, response_size);
        longResponse = false;
        longResponseData = false;
        return true;
    } 

    // check if the previous command was change db
    if (lastCommandId == MYSQL_DB)
    {
        db_name = db_new_name;

        //load new db settings
        db = dbmap_find(iProxyId, db_name, "mysql");
    }

    // could happen when using "SET command"
    if (type == MYSQL_SRV_OK && response_size <= max_response_size)
    {
        // mysql ok packet
        response_in.pop(response, response_size);
        longResponse = false;
        longResponseData = false;
        return true;
    }

    //debug
    logevent(SQL_DEBUG, "dump response, size %d\n", max_response_size);
    loghex(SQL_DEBUG, data, max_response_size);
    response_in.pop(response, max_response_size);
    return true;


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
        temp = 0;

        if (header_size + temp + 5 < max_response_size &&
            data[header_size+temp+4] != MYSQL_SRV_ENDROW)
        {
            //add size of the last packet
            //header_size += temp;

            //sometimes it sends more data in the header
            while (header_size + temp < max_response_size - 5 &&
                   data[header_size + temp+4] != MYSQL_SRV_ENDROW)
            {
                header_size += temp;
                temp = (data[header_size+2] <<16 |
                        data[header_size+1] << 8 |
                        data[header_size+0]) + 4;
                logevent(NET_DEBUG, "Additional Header size %d, type %d.\n",
                         temp, data[header_size + 4]);
            }
        }
        // check if we have space left for the end of the header packet
        if (header_size + temp + 5 > max_response_size)
        {
            // read more data to decide
            logevent(NET_DEBUG, "Read more data to decide. (%d-%d)\n", 
                header_size + temp + 5, max_response_size);
            return false;
        }
        // now must go end of fields packet
        if (data[header_size+temp+4] == MYSQL_SRV_ENDROW)
        {
            // add size of the last packet
            header_size += temp;

            logevent(NET_DEBUG, "End of fields packet %d-%d\n",
            header_size, max_response_size);

            temp = (data[header_size+2] <<16 |
                    data[header_size+1] << 8 |
                    data[header_size+0]) + 4;

            logevent(NET_DEBUG, "Last Field size %d.\n", temp);
        } else {
            logevent(NET_DEBUG, "Packet end/type missmatch %d.\n", 
                     data[header_size+temp+4]);
            return false;
        }
        if (header_size + temp <= max_response_size)
        {
            header_size += temp;
            if (lastCommandId != MYSQL_FIELD_LIST)
                longResponseData = true;
            longResponse = false;
        
            logevent(NET_DEBUG, "End of header-start of data (left %d bytes).\n", max_response_size - header_size);
            full_size = header_size;
        } else {
            logevent(NET_DEBUG, "Failed to get end of header in this packet\n");
            return false;
        }
    }

    if (longResponseData == true && full_size + 5 <= max_response_size)
    {
        logevent(NET_DEBUG, "Data packet, size left %d\n", max_response_size - full_size);

        row_size = (data[full_size+2] <<16 |
                    data[full_size+1] << 8 |
                    data[full_size+0]) + 4;
    
        while ( (full_size + row_size + 5 <= max_response_size) && 
                (data[full_size+4] != MYSQL_SRV_ENDROW))
        {
            // data packet
            full_size += row_size;
            row_size = (data[full_size+2] <<16 |
                        data[full_size+1] << 8 |
                        data[full_size+0]) + 4;
        }

        if (data[full_size+4] == MYSQL_SRV_ENDROW &&
            full_size + row_size <= max_response_size)
        {
            full_size += row_size;
            longResponseData = false;
            logevent(NET_DEBUG, "End of long response.\n");
        } else if (full_size + row_size <= max_response_size)
        {
        full_size += row_size;
        logevent(NET_DEBUG, "End not reached, left in packet %d\n", max_response_size-full_size);
        } else {
            logevent(NET_DEBUG, "End not reached, next row size: %d, left in packet %d\n", row_size, max_response_size-full_size);
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
