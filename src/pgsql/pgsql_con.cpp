//
// GreenSQL MySQL Connection class code.
//
// Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
// License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
//

#include "pgsql_con.hpp"

#include "../normalization.hpp"
#include "../config.hpp"
#include "../dbmap.hpp"

#include "../misc.hpp" // for str_lowercase
#include "../alert.hpp" // for logalert


static SQLPatterns pgsql_patterns;

bool pgsql_patterns_init(std::string & path)
{
    std::string file = path + "pgsql.conf";

    return pgsql_patterns.Load(file);
}

SQLPatterns * PgSQLConnection::getSQLPatterns()
{
    return & pgsql_patterns;
}

PgSQLConnection::PgSQLConnection(int id): Connection(id)
{
    first_request = true;
    longResponse = false;
    longResponseData = false;
    db_type = "pgsql";
    db = dbmap_default(id, db_type.c_str());
}

PgSQLConnection::~PgSQLConnection()
{
}

bool PgSQLConnection::checkBlacklist(std::string & query, std::string & reason)
{
    //GreenSQLConfig * conf = GreenSQLConfig::getInstance();
    bool ret = false;
    bool bad = false;
    if (db->CanAlter() == false)
    {
        ret = pgsql_patterns.Match( SQL_ALTER,  query );
        if (ret == true)
        {
            reason += "Detected attempt to change database/table structure.\n";
        logevent(DEBUG, "Detected attempt to change database/table structure.\n");
        bad = true;
        }
    }
    if (db->CanDrop() == false)
    {
        ret = pgsql_patterns.Match( SQL_DROP,  query );
        if (ret == true)
        {
            reason += "Detected attempt to drop database/table/index.\n";
        logevent(DEBUG, "Detected attempt to drop database/table.\n");
        bad = true;
        }
    }
    if (db->CanCreate() == false)
    {
        ret = pgsql_patterns.Match( SQL_CREATE,  query );
        if (ret == true)
        {
            reason += "Detected attempt to create database/table/index.\n";
        logevent(DEBUG, "Detected attempt to create database/table/index.\n");
        bad = true;
        }
    }
    if (db->CanGetInfo() == false)
    {
        ret = pgsql_patterns.Match( SQL_INFO,  query );
        if (ret == true)
        {
            reason += "Detected attempt to discover db internal information.\n";
        logevent(DEBUG, "Detected attempt to discover db internal information.\n");
        bad = true;
        }
    }
    return bad;
}
bool PgSQLConnection::parseRequest(std::string & request, bool & hasResponse)
{
    unsigned int full_size = request_in.size();

    if (full_size < 5)
        return true;
    
	size_t request_size,start =0;
    const unsigned char * data = request_in.raw();
	logevent(NET_DEBUG,"request: full size: %d\n",full_size);
	loghex(NET_DEBUG,data,full_size);
    //check if we got full packet
    if (full_size == 8)
    {
        // wait for more data to decide
        unsigned int client_flags = (data[7] << 24 | data[6]<<16 | data[5] << 8 | data[4]);
        if (client_flags & PGSQL_CON_SSL)
            logevent(SQL_DEBUG, "Client uses SSL encryption\n");
        //disable ssl encription not supported yet
		if (response_in.size() != 0)
		{
			// push it to the server response parsing queue
			response_in.append("N", 1);
		} else {
			// push it to the client
			response_out.append("N", 1);
		}
		hasResponse = true;
        request_in.pop(request,full_size);
		request.clear();
        return true;
    }
	if (first_request) {
		request_size = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]; 
		logevent(SQL_DEBUG, "Request size: %d, first request\n",request_size);
		if (request_size > full_size)
			return true;
		if (!parse_request(data, request_size,hasResponse))
			return false;
		request_in.pop(request, full_size);
		return true;
	}

	request_size = (data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4])+1; 
	logevent(NET_DEBUG, "request: packet size expected %d bytes (received %d)\n", full_size - start, request_size);
	while(start + request_size <= full_size)
	{
		parse_request(data+start, request_size,hasResponse);
		start += request_size;
		if(start == full_size)
			break;
		if (start+5 > full_size)
		{
			request_in.pop(request, start);
			return true;
		}
		request_size = (data[start+1] << 24 | data[start+2] << 16 | data[start+3] << 8 | data[start+4])+1; 
		logevent(NET_DEBUG, "request: packet size expected %d bytes (received %d)\n", full_size - start, request_size);
	}
	if(request_in.size())
		request_in.pop(request, start);
    return true;
}
void ErrorResponseHandling(const unsigned char * data,unsigned int start = 6)
{
	logevent(SQL_DEBUG, "PGSQL ERROR RESPONSE command\n");
	string tmp;
	unsigned int len;
	tmp.append((const char*)data+start);
	if(!tmp.empty())
		logevent(SQL_DEBUG,"Error type: %s\n",tmp.c_str());
	len = tmp.size()+start + 2;
	tmp ="";
	tmp.append((const char*)data+len);
	if(!tmp.empty())
		logevent(SQL_DEBUG,"Error number: %s\n",tmp.c_str());
	len += tmp.size()+ 2;
	tmp ="";
	tmp.append((const char*)data+len);
	if(!tmp.empty())
		logevent(SQL_DEBUG,"Error name: %s\n",tmp.c_str());
}
bool PgSQLConnection::parseResponse(std::string & response)
{
	size_t full_size = response_in.size();
	const unsigned char * data = response_in.raw();
	size_t used = 0;

	first_response = false;

	logevent(NET_DEBUG, "response: full size %d bytes\n", full_size); //-V111
	loghex(NET_DEBUG, data, full_size);

	if (!( first_request || data[0] == PGSQL_SRV_PASSWORD_MESSAGE || data[0] == PGSQL_ERROR_RESPONSE || SecondPacket))
	{
		response_in.pop(response, full_size);
		return true;
	}
	if(!parse_response(data, used, full_size, response))
		return true;
	// add data to cache
	response_in.pop(response, used);
	return true;
}

bool PgSQLConnection::blockResponse(std::string & response)
{
    // we will return an empty result
    char data[] = {'T',0,0,0,6,0,0,'C',0,0,0,11,'S','E','L','E','C','T',0,'Z',0,0,0,5,73}; // OK response
    response.append(data,25);
    return true;
}
void PgSQLConnection::blockParseResponse(std::string & response)
{
    char error[] = {'E',0,0,0,42,'S','E','R','R','O','R',0,'C',4,2,5,0,1,0,'M'};
    char ready[] = {0,0,'Z',0,0,0,5,73};
    response.append(error, 20);
    response.append("Access Rule Violation");
    response.append(ready,8);
}
static bool pg_parse_protocol_params(const unsigned char * data, size_t request_size, size_t & ind, const char * const option, unsigned char option_len, std::string & result) {
	if ((ind + option_len) > request_size) {
		ind = request_size;
		return false;
	}
	if (!memcmp(data, option, option_len)) {
		ind += option_len+1;
		while (ind < request_size && data[ind] != '\0' ) ++ind;
		return false;
	}
	ind += option_len+1;
	unsigned int i = 0;
	while (ind < request_size && data[ind] != '\0') {
		++ind; ++i;
	}
	if (i > 0) 
		result.assign((const char*)data + (ind - i), i);
	++ind;
	return true;
}
unsigned char PGSQL_CANCEL[] = {0x00, 0x00, 0x00, 0x0F, 0x04, 0xD2, 0x16, 0xDF};
bool PgSQLConnection::parse_request(const unsigned char * data, size_t request_size,bool & hasResponse) {

	std::string response;
	std::string prepareName;

	unsigned int request_type = data[0];

	size_t ind = 1;

	if (first_request) 
	{
		if (request_size < 13) {
			logevent(SQL_DEBUG,"PGSQL Unsupported protocol version\n");
			loghex(SQL_DEBUG, data, 12);
			return false;
		}

		db_name.clear();
		db_user.clear();

		logevent(SQL_DEBUG, "PGSQL STARTUP MESSAGE command\n");
		//protocol version supporter 3.0
		proto_version.append((const char*)data + 4,2);
		proto_version.append('.', 1);
		proto_version.append((const char*)data + 6,2);

		ind = 8;
		while (ind < request_size) {
			switch (data[ind]) 
			{
			case 'u':
				if (pg_parse_protocol_params(data, request_size, ind, "user", 4, db_user)) 
					logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
				break;
			case 'd':
				if (pg_parse_protocol_params(data, request_size, ind, "database", 8, db_name)) {
					logevent(SQL_DEBUG, "DATABASE: %s\n", db_name.c_str());
					db = dbmap_find(iProxyId, db_name, "pgsql");
				}
				break;
			case 'o':
				if (pg_parse_protocol_params(data, request_size, ind, "options", 7, db_options)) {
					logevent(SQL_DEBUG, "OPTIONS: %s\n", db_options.c_str());
				}
				break;
			default:
				while (ind < request_size && data[ind] != '\0' ) ++ind;
				if (data[ind] == '\0') ++ind;
				break;
			}
		}
		if(db->db_name != db_name)
		{
			logevent(SQL_DEBUG, "find db name\n");
			db = dbmap_find(iProxyId, db_name, "pgsql");
		}
		first_request = false;
		return true;
	}

	if (request_size == 16 && memcmp(data, PGSQL_CANCEL, 8)) {
		logevent(SQL_DEBUG, "PGSQL CANCEL REQUEST command\n");
		return true;
	}

	//simple messages
	switch(request_type) 
	{
	case PGSQL_PASSWORD: 
		logevent(SQL_DEBUG, "PGSQL Password Message\n");
		SecondPacket = true;
		return true;
	case PGSQL_QUIT: 
		logevent(SQL_DEBUG, "PGSQL QUIT command\n");
		return true;
	case PGSQL_CLOSE: 
		logevent(SQL_DEBUG, "PGSQL CLOSE command\n");
		return true;
	case PGSQL_COPY_DATA: 
		logevent(SQL_DEBUG, "PGSQL COPY DATA backend command\n");
		return true;
	case PGSQL_EXECUTE: 
		logevent(SQL_DEBUG, "PGSQL EXECUTE command\n");
		return true;
	case PGSQL_FLUSH: 
		logevent(SQL_DEBUG, "PGSQL FLUSH command\n");
		return true;
	case PGSQL_FUNC_CALL: 
		logevent(SQL_DEBUG, "PGSQL FUNCTION CALL command\n");
		return true;
	case PGSQL_SYNC:
		logevent(SQL_DEBUG, "PGSQL SYNC command\n");
		return true;
	case PGSQL_COPY_DONE:
		logevent(SQL_DEBUG, "PGSQL COPY DONE frontend command\n");
		return true;
	case PGSQL_COPY_FAIL:
		logevent(SQL_DEBUG, "PGSQL COPY FAIL command\n");
		return true;
	case PGSQL_DESCRIBE:
		logevent(SQL_DEBUG, "PGSQL DESCRIBE command\n");
		return true;
	}
	// we got command? 
	switch(request_type)
	{
	case PGSQL_QUERY:
		longResponse = true;
		longResponseData = false;
		original_query.clear();
		original_query.append((char *)data + 5);
		logevent(SQL_DEBUG, "QUERY command[%s]: %s\n", db_name.c_str(), original_query.c_str()); //-V111

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
		break;

	case PGSQL_BIND: 
		{
			logevent(SQL_DEBUG, "PGSQL BIND command\n");
			prepareName.append((char *)data + 5);
			if (!prepareName.empty())
			{
				logevent(SQL_DEBUG, "Destination portal name: \"%s\"\n",prepareName.c_str());
			}
			else
			{
				logevent(SQL_DEBUG, "Unnamed destination portal\n");
			}
			original_query.clear();
			original_query.append((char *)data + strlen(prepareName.c_str())+6);
			if (!original_query.empty())
			{
				logevent(SQL_DEBUG, "Prepared statement name: \"%s\"\n", original_query.c_str());
			}
			else
			{
				logevent(SQL_DEBUG, "Unnamed prepared statement\n");
			}
			size_t ind = original_query.size() + 6;
			size_t parameterNum = (data[ind] << 8) | data[ind + 1]; //-V101
			logevent(SQL_DEBUG, "Number of parameter format codes: %d\n",parameterNum);
			ind += 2;
			for(size_t i=0; i < parameterNum;++i,ind++)
				logevent(SQL_DEBUG, "%d parameter format codes is: %s\n",i+1,data[ind]? "binary" : "text"); //-V111

			if(parameterNum == 0)
				ind++;

			parameterNum = data[ind] << 8 | data[ind + 1]; //-V101
			logevent(SQL_DEBUG, "Number of parameters needed by the query is: %d\n",parameterNum);
			ind += 2;
			for(size_t i=0;i < parameterNum;++i)
			{
				size_t num = (data[ind] << 24 | data[ind+1] << 16 | data[ind+2] << 8 | data[ind+3]) & 0xFFFFFFFF; //-V101
				if (num == 0xFFFFFFFF)// temp is null
				{
					logevent(SQL_DEBUG, "(%d) Value is: NULL\n",i+1);
					ind+= 4;
					continue;
				}
				logevent(SQL_DEBUG, "Length of the (%d) parameter value is: %d\n",i+1,num); //-V111
				ind+=4;
				ind += num;
			}

			parameterNum = data[ind] << 8 | data[ind + 1]; //-V101
			logevent(SQL_DEBUG, "number of result-column format codes is: %d\n",parameterNum); //-V111
			ind += 2;
			for(size_t i=0;i < parameterNum;++i,ind++)
				logevent(SQL_DEBUG, "%d result-column format codes is: %s\n",i+1,data[ind]? "binary" : "text");

			ind++;
			if(data[ind] == PGSQL_DESCRIBE)
				logevent(SQL_DEBUG, "PGSQL DESCRIBE command\n");
			if(data[ind] == PGSQL_EXECUTE)
				logevent(SQL_DEBUG, "PGSQL EXECUTE command\n");
			if(data[ind] == PGSQL_SYNC)
				logevent(SQL_DEBUG, "PGSQL SYNC command\n");
		}
		break;
	case PGSQL_PARSE: 
		{
			if(data[1] != 0)
				break;
			logevent(SQL_DEBUG, "PGSQL PARSE command\n");
			prepareName.append((char *)data + 5);
			logevent(SQL_DEBUG, "Prepared statement name: %s\n",prepareName.c_str());
			original_query.clear();
			original_query.append((char *)data + strlen(prepareName.c_str())+6);
			logevent(SQL_DEBUG, "Prepared query name: %s\n", original_query.c_str());

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
		break;

	default:
		logevent(SQL_DEBUG, "UNKNOWN COMMAND\n");
		loghex(SQL_DEBUG, data, request_size);
		break;
	} 
	return true;
}
bool PgSQLConnection::parse_response(const unsigned char * data, size_t& used_size,size_t& max_response_size, std::string & response)
{
	unsigned int type;
	size_t start=0;
	size_t response_size;
	size_t value_len;
	size_t ind;
	ErrorType error_type = SQL_DEBUG;

	if (max_response_size < 3)
	{
		logevent(NET_DEBUG, "received %d bytes of response\n", max_response_size); //-V111
		return false;
	}

	string str_value;
	while(start + 4 < max_response_size)
	{
		type = data[start];
		response_size = data[1+start] << 24 | data[2+start] << 16 | data[3+start] << 8 | data[4+start];
		if((start + response_size+1) > max_response_size || response_size == 0)
		{
			logevent(NET_DEBUG, "response: more packets...\n");
			return false;
		}
		logevent(NET_DEBUG, "packet size expected %d bytes (received %d)\n", max_response_size - start, response_size+1);
		switch(type)
		{
		case PGSQL_SRV_PASSWORD_MESSAGE:
			{
				size_t auth = data[start + 4];
				type = data[start + 8];
				if(auth == PGSQL_AUTH_MD5 && type == 5)
				{
					logevent(NET_DEBUG, "Client uses MD5 Password\n");
				}
				else
				{
					switch(type)
					{
					case 0:
						{
							logevent(NET_DEBUG, "response: PGSQL Authentication Ok\n");
							size_t type = data[start + 9];
							if(type == 83)
							{
								ind = 0; //response_in.FindSubString("server_version");
								if(ind >0)
								{
									db_srv_version.append((const char*)data + start + ind + strlen("server_version")+1,strlen((const char*)data + start + ind+strlen("server_version")+1));
								}
							}
							if(data[9] == PGSQL_EXECUTE)
								ErrorResponseHandling(data,start + 15);
						}
						break;
					case 2:logevent(NET_DEBUG, "Client uses Kerberos V5 Password\n");break;
					case 3:logevent(NET_DEBUG, "Client uses Clear text Password\n");break;
					case 6:logevent(NET_DEBUG, "Client uses SCM Credential Password\n");break;
					case 7:logevent(NET_DEBUG, "Client uses GSS Password\n");break;
					case 9:logevent(NET_DEBUG, "Client uses SSPI Password\n");break;
					default:logevent(NET_DEBUG, "Client uses UNKNOWN Password type\n");break;
					}
				}
			}
			break;
		case PGSQL_SRV_GETROW:
			logevent(error_type, "response: PGSQL ROW DESCRIPTION command\n");
			break;
		case PGSQL_ROW_DATA:
			logevent(error_type, "response: PGSQL ROW DATA command\n");
			break;
		case PGSQL_BECKEND_KEY_DATA: 
			logevent(error_type, "response: PGSQL BECKEND KEY DATA command\n");
			logevent(error_type, "Backend process ID: %d\n", data[5+start] << 24 | data[6+start] << 16 | data[7+start] << 8 | data[8+start]);
			break;
		case PGSQL_BIND_COMPLETE:
			logevent(error_type, "response: PGSQL BIND COMPLETE command\n");
			break;
		case PGSQL_CLOSE_COMPLETE:
			logevent(error_type, "response: PGSQL CLOSE COMPLETE command\n");
			break;
		case PGSQL_COMMAND_COMPLETE:
			logevent(error_type, "response: PGSQL COMPLETE command\n");
			break;
		case PGSQL_COPY_DATA: 
			logevent(error_type, "response: PGSQL COPY DATA frontend command\n");
			break;
		case PGSQL_COPY_DONE:
			logevent(error_type, "response: PGSQL COPY DONE frontend command\n");
			break;
		case PGSQL_COPY_IN_RESPONSE:
			logevent(error_type, "response: PGSQL COPY IN RESPONSE command\n");
			break;
		case PGSQL_FLUSH:
			logevent(error_type, "response: PGSQL COPY OUT RESPONSE command\n");
			break;
		case PGSQL_EMPTY_QUERY_RESPONSE:
			logevent(error_type, "response: PGSQL EMPTY QUERY RESPONSE command\n");
			break;
		case PGSQL_FUN_CALL_RESPONSE:
			logevent(error_type, "response: PGSQL FUNCTION CALL RESPONSE command\n");
			break;
		case PGSQL_NO_DATA:
			logevent(error_type, "response: PGSQL NO DATA command\n");
			break;
		case PGSQL_ERROR_RESPONSE:
		case PGSQL_NOTICE_RESPONSE:
			{
				if(type == PGSQL_NOTICE_RESPONSE)
					logevent(error_type, "response: PGSQL NOTICE RESPONSE command\n");
				else
				{
					logevent(error_type, "response: PGSQL ERROR RESPONSE command\n");
					ErrorResponseHandling(data,start + 6);
				}
			}
			break;
		case PGSQL_NOTIF_RESPONSE:
			logevent(error_type, "response: PGSQL NOTIFICATION RESPONSE command\n");
			break;
		case PGSQL_PARAM_STATUS:
			logevent(error_type, "response: PGSQL PARAMETER STATUS command\n");
			break;
		case PGSQL_PARSE_COMPLETE:
			logevent(error_type, "response: PGSQL PARSE COMPLETE command\n");
			break;
		case PGSQL_PORTAL_SUSPENDED:
			logevent(error_type, "response: PGSQL PORTAL SUSPENDED command\n");
			break;
		case PGSQL_READY_FOR_QUERY:
			logevent(error_type, "response: Ready For Query\n");
			break;
		default:
			logevent(error_type, "response: Unknown type: %d\n",type);
		}
		start += response_size + 1;
	}
	used_size = start;
	if(SecondPacket)
		SecondPacket = false;
	return true;
}