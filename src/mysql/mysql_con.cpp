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
	StartResponse = false;
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
    size_t full_size = request_in.size();
    if (full_size < 3)
        return true;

    const unsigned char * data = request_in.raw();
    size_t request_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
	size_t start = 0;

    //check if we got full packet
    if (request_size > full_size)
    {
        // wait for more data to decide
        return true;
    }
	while(start+ request_size <= full_size)
	{
		ParseRequestPacket(data, request_size,hasResponse);
		start += request_size;
		if(start == full_size)
			break;
		if (start + 7 > full_size)
		{
			request_in.pop(request, start);
			return true;
		}
		data = request_in.raw();
		request_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
	}
	if(request_in.size())
		request_in.pop(request, start);
	return true;
}

bool MySQLConnection::parseResponse(std::string & response)
{
    size_t full_size = response_in.size();
    size_t start = 0;
    size_t header_size;
    if (full_size < 3)
    {
       logevent(NET_DEBUG, "received %d bytes of response\n", full_size);
       return false;
    }
    const unsigned char * data = response_in.raw();
    // max packet size is 16 MB
    size_t response_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;
    unsigned int packet_id = data[3];
    unsigned int type = data[4];

    logevent(NET_DEBUG, "packet size expected %d bytes (received %d)\n", 
            response_size, full_size);
    logevent(SQL_DEBUG, "server packet %d\n", packet_id);

    if (!((StartResponse && data[4] == MYSQL_SRV_ERROR) || lastCommandId == MYSQL_DB || first_request || SecondPacket)) //-V112
    {
        response_in.pop(response, full_size);
        StartResponse = false;
        return true;
    }
    StartResponse = false;

    if (full_size < 7)
    {
        if (first_request || SecondPacket)
	{
		response_in.pop(response, full_size);
		return true;
	}
	logevent(NET_DEBUG, "response: received %d bytes of response\n", full_size); //-V111
	loghex(NET_DEBUG, data, full_size);
	return false;
    }
    response_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;

    while(start + response_size <= full_size && full_size)
    {
	logevent(NET_DEBUG, " response: packet size expected %d bytes (received %d)\n",start + response_size, full_size);
	size_t header_size = 0;
	// request's equivalent response
	if(!ParseResponsePacket(data + start,response_size,full_size - start,response,header_size))
	{
		logevent(NET_DEBUG, "response: more packets...\n");
		if(SecondPacket)
			SecondPacket = false;
		return true;
	}
	if(SecondPacket)
	 	SecondPacket = false;
	start += header_size? header_size : response_size;
	if(start < full_size)
			response_size = (data[2]<<16 | data[1] << 8 | data[0]) + 4;

		if((start+ response_size) > full_size)
			break;
	}

	if(response_in.size())
		response_in.pop(response, start);

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
bool MySQLConnection::ParseRequestPacket(const unsigned char* data,size_t& request_size,bool& hasResponse)
{
	size_t packet_id = data[3];
	size_t type = data[4]; //-V112
	size_t max_user_len;
	size_t user_len;
	size_t db_name_len;
	size_t pwd_len = 0;
	std::string response;
	logevent(SQL_DEBUG, "---------------------Client packet %d--------------------------\n", packet_id);
	//loghex(NET_DEBUG,data,request_size);
	//check if we got full packet

	// this is a first request in the stream
	// it must contain login information. 
	if (first_request == true)
	{
		//loghex(SQL_DEBUG, data, request_size);
		if (request_size < 10)
			return false;

		// client flags can be 2 or 4 bytes long
		unsigned int client_flags = (data[7] << 24 | data[6]<<16 | 
			data[5] << 8 | data[4]); //-V112

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
		std::string old_db = db_name;
		db_name.clear();
		db_user.clear();
		if ((client_flags & MYSQL_CON_PROTOCOL_V41) && request_size > 36)
		{
			user_len = strlen((const char *)data+36);
			if (user_len + 36 > request_size)
			{
				user_len = request_size - 37;
				db_user.append((const char*)data+36, user_len);
				logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str()); //-V111
			} else {
				//check if we have space for dnbame and pwd
				if (user_len + 36 + 1 >= request_size)
					return false;
				db_user = (const char *)data+36;
				logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str()); //-V111

				size_t pwd_size = data[user_len+36+1];
				if (pwd_size == 0)
				{
					db_name_len = strlen((const char*)data+user_len+36+2);
					if (db_name_len > 0)
					{
						if (db_name_len > request_size-user_len-36-3)
							db_name_len = request_size-user_len-36-3;
						db_name.append((const char*)data+user_len+36+2, db_name_len);
						logevent(SQL_DEBUG, "DATABASE2: %s\n", db_name.c_str());
						db = dbmap_find(iProxyId, db_name, "mysql");
					}
				}
				else if (user_len+36+pwd_size+2 < request_size)
				{
					db_name_len = strlen((const char*)data+user_len+36+pwd_size+2);
					if (db_name_len > request_size-user_len-36-pwd_size-3)
						db_name_len = request_size-user_len-36-pwd_size-3;
					db_name.append((const char *)data+user_len+36+pwd_size+2, db_name_len);
					logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());
					db = dbmap_find(iProxyId, db_name, "mysql");
				}
			}
		}
		else if (request_size > 10)
		{
			user_len = strlen((const char *)data+9);
			if (user_len > request_size - 9)
				user_len = request_size - 9;
			if (user_len != 0)
			{
				db_user.append((const char *) data+9, user_len); 
				logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
			}

			size_t temp= 9 + user_len + 1;
			if (temp > request_size )
			{
				first_request = false;
				return false;
			}
			while(temp < request_size && data[temp] != '\0')
				temp++;
			if (temp == request_size)
			{
				first_request = false;
				return true;
			}
			temp++;
			if (temp == request_size)
			{
				first_request = false;
				return true;
			}
			size_t end = temp;
			while (end < request_size && data[end] != '\0')
				end++;

			db_name.clear();
			db_name.append((const char*)data+temp, end-temp);
			db = dbmap_find(iProxyId, db_name, "mysql");
			logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());

		}
		if(db->db_name != db_name)
			db = dbmap_find(iProxyId, db_name, "mysql");
		first_request = false;
		SecondPacket = true;
		// we do not have any additional commands in first packet.
		// just send everything we got so far.
		return true;
	}
	lastCommandId = (MySQLType)(int)(data[4]);
	if (type == MYSQL_QUERY ||
		type == MYSQL_FIELD_LIST ||
		//type == MYSQL_STATISTICS ||
		type == MYSQL_PROCESS_INFO ||
		type == MYSQL_STMT_FETCH)
	{
		logevent(SQL_DEBUG, "long response is true\n");
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
		db_new_name.clear();
		db_new_name.append((const char *)data+5, request_size-5);
		logevent(SQL_DEBUG, "DATABASE: %s\n", db_new_name.c_str());
	} 
	else if (type == MYSQL_QUERY) 
	{
		// query must not be empty
		// otherwise system can crash
		if (data[5] == '\0')
		{
			return true;
		}
		StartResponse = true;
		size_t max_query_len = request_size - 5;
		original_query.clear();
		original_query.append((char *)data+5, max_query_len);
		if ( check_query(original_query) == false)
		{
			// bad query - block it
			original_query.clear(); // do not send it to backend server
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
		}
	} 
	else if ((type == MYSQL_CREATE_DB || type == MYSQL_DROP_DB || type == MYSQL_FIELD_LIST) && request_size > 5)
	{
		// we will emulate create database command and run it as if it is a regular command
		if (type == MYSQL_CREATE_DB)
			original_query = "create database '";
		else if (type == MYSQL_DROP_DB)
			original_query = "drop database '";
		else if (type == MYSQL_FIELD_LIST)
			original_query = "show fields from '";
		original_query.append((const char *)data+5, request_size-5);
		original_query.push_back('\'');
		StartResponse = true;

		if ( check_query(original_query) == false)
		{
			// bad query - block it
			original_query.clear(); // do not send it to backend server
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
	} 
	else if (type == MYSQL_PROCESS_INFO)
	{
		logevent(SQL_DEBUG, "MYSQL PROCESSLIST command\n");
		original_query = "show processlist";
		StartResponse = true;
		if ( check_query(original_query) == false)
		{
			// bad query - block it
			original_query.clear(); // do not send it to backend server
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
		size_t user_len = strlen((const char*)data+5);
		if (user_len > max_user_len)
			user_len = max_user_len;
		std::string old_user_name = db_user;
		db_name = "";
		db_user.assign((const char*)data+5, max_user_len);
		pwd_len = 0;
		if (mysql41 == true)
		{
			pwd_len = data[5+user_len];
		} else {
			pwd_len = strlen((const char*) data+5+user_len);
		}
		if (user_len + pwd_len + 5 < request_size )
		{
			db_name_len = strlen((const char *)data+5+user_len+pwd_len);
			if (db_name_len + 5+user_len+pwd_len > request_size)
				db_name_len = request_size - 5 - user_len - pwd_len;
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
			return true;
		}
		size_t max_query_len = request_size - 5;
		original_query.clear();
		original_query.append((const char *)data+5, max_query_len);
		logevent(SQL_DEBUG, "MYSQL PREPARE QUERY[%s]: %s\n", db_name.c_str(), original_query.c_str()); //-V111
		StartResponse = true;

		if ( check_query(original_query) == false)
		{
			// bad query - block it
			original_query.clear(); // do not send it to backend server
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
		}
	} else if (type == MYSQL_STMT_EXEC) {
		size_t statement_id = (data[8] << 24 | data[7]<<16 |  //-V101
			data[6] << 8 | data[5]);
		logevent(SQL_DEBUG, "MYSQL STMT EXECUTE: %u\n", statement_id); //-V111
	} else if (type == MYSQL_LONG_DATA)
	{
		logevent(SQL_DEBUG, "MYSQL LONG DATA command\n");
	} else if (type == MYSQL_STMT_CLOSE)
	{
		size_t statement_id = (data[8] << 24 | data[7]<<16 |  //-V101
			data[6] << 8 | data[5]);
		logevent(SQL_DEBUG, "MYSQL STMT CLOSE: %u\n", statement_id);
	} else if (type == MYSQL_STMT_RESET)
	{
		size_t statement_id = (data[8] << 24 | data[7]<<16 |  //-V101
			data[6] << 8 | data[5]);
		logevent(SQL_DEBUG, "MYSQL STMT RESET: %u\n", statement_id); //-V111
	} else if (type == MYSQL_SET_OPTION)
	{
		logevent(SQL_DEBUG, "MYSQL SET OPTION command\n");
	} else if (type == MYSQL_STMT_FETCH)
	{
		size_t statement_id = (data[8] << 24 | data[7]<<16 |  //-V101
			data[6] << 8 | data[5]);
		logevent(SQL_DEBUG, "MYSQL STMT FETCH: %u\n", statement_id); //-V111
	} else {
		logevent(SQL_DEBUG, "UNKNOWN COMMAND\n");
		loghex(SQL_DEBUG, data, request_size);   
	}
	return true;
}
bool MySQLConnection::ParseResponsePacket(const unsigned char* data,size_t& response_size,size_t max_response_size,std::string& response,size_t& header_size)
{
	unsigned int packet_id = data[3];
	unsigned int type = data[4];
	logevent(SQL_DEBUG, "server packet %d\n", packet_id);
	if (first_request == true && response_size > 23 && response_size <= max_response_size)
	{
		//loghex(NET_DEBUG,data,max_response_size);
		logevent(SQL_DEBUG, "first server packet \n");
		//unsigned int protocol_version = data[5];
		size_t max_srv_len = response_size - 5;
		size_t srv_len = strlen((const char*)data+5);
		if (srv_len > max_srv_len)
			srv_len = max_srv_len;
		db_srv_version.append((const char*)data+5, srv_len);

		if (srv_len + 20 > response_size)
		{
			// strange packet
			return true;
		}
		//unsigned int thread_id = (data[srv_len+9] << 24 | data[srv_len+8] << 16 | data[srv_len+7] << 8 | data[srv_len+6]);
		// next 8 bytes - scramble data
		// next byte - zero
		unsigned int server_flags = (data[srv_len+20] << 8 | data[srv_len+19]);
		//unsigned int server_language = data[srv_len+21];
		//unsigned int server_status = (data[srv_len+23] << 8 | data[srv_len+22]);
		// next we have additional scramble data

		response_in.pop(response,response_size);
		if (server_flags & MYSQL_CON_PROTOCOL_V41)
		{
			logevent(SQL_DEBUG, "Server supports protocol 4.1\n");
		}
                // we will disable compression and SSL support
		if (server_flags & MYSQL_CON_COMPRESS)
		{
		    logevent(NET_DEBUG, "Disable mysql server traffic compression capability as GreenSQL does not supports it.\n");
		    server_flags = server_flags & ( ~ (unsigned int)(MYSQL_CON_COMPRESS));
                }
                if (server_flags & MYSQL_CON_SSL)
                {
                    logevent(NET_DEBUG, "Disable mysql SSL traffic encryption as GreenSQL does not supports it.\n");
                    server_flags = server_flags & ( ~ (unsigned int)(MYSQL_CON_SSL));
                }
		// we will disable compression and SSL support
		//if (!ssl_enabled) 
		//    server_flags = server_flags  & ( ~ (unsigned int)(MYSQL_CON_SSL));

		response[srv_len+19] = (server_flags & 0xff);
		response[srv_len+20] = ((server_flags >> 8) & 0xff);
		data = response_in.raw();
		max_response_size = response_in.size();
		return true;
	}


	// error message can be up to 512 chars long
	if (type == MYSQL_SRV_ERROR && response_size > 7 && response_size < 600 && response_size <= max_response_size)
	{
		//unsigned int server_error = (data[6] << 8 | data[5]);
		size_t max_error_len = response_size - 7;
		size_t error_len = strlen((const char*)data+7);
		if (error_len > max_error_len)
			error_len = max_error_len;

		if (error_len > 0)
		{
			std::string full_error = "";
			full_error.append((const char*)data+7, error_len);
			logevent(SQL_DEBUG, "SQL_ERROR: %s\n", full_error.c_str());
		}
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
	if (type == MYSQL_SRV_OK && response_size > 7 && response_size < 600 && response_size <= max_response_size)
	{
		logevent(SQL_DEBUG, "OK packet \n");
		// mysql ok packet
		//response_in.pop(response, response_size);
		return true;
	}

	if (longResponse == false && 
		longResponseData == false)
	{
		return true;
	}
	header_size = response_size;
	return true;
}
