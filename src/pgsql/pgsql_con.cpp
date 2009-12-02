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

    if (full_size < 3)
        return true;

    std::string query = "";
    std::string response = "";
    std::string prepareName = "";
    

    const unsigned char * data = request_in.raw();
    //////////////////////////////////////////////////////////////////////////
    //debug
    //loghex(SQL_DEBUG, data, full_size);
    //////////////////////////////////////////////////////////////////////////

    unsigned int request_type = data[0];
    unsigned int index;

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
		request = "";
        return true;
    }
    if(request_type ==  PGSQL_PASSWORD && data[1] == 0)
    {
        logevent(SQL_DEBUG, "PGSQL Password Message\n");
        request_in.pop(request,full_size);
        return true;
    }
    // this is a first request in the stream
    // it must contain login information. 
    if (first_request == true)
    {
        if (full_size < 8)
          return false;

        db_name = "";
        db_user = "";

        if (full_size > 13)
        {
            logevent(SQL_DEBUG, "PGSQL STURTUP MESSAGE command\n");
            //protocol version supporter 3.0
            proto_version.append((const char*)data + 4,2);
            proto_version.append(".");
            proto_version.append((const char*)data + 6,2);

            index = request_in.FindSubString("user");
            if(index != std::string::npos)
            {
                index += (unsigned)strlen("user");
                db_user.append((const char*)data + index + 1);
                logevent(SQL_DEBUG, "USERNAME: %s\n", db_user.c_str());
            }
            index = request_in.FindSubString("database");
            if (index != std::string::npos)
            {
                index += (unsigned)strlen("database");
                db_name.append((const char*)data + index + 1);
                logevent(SQL_DEBUG, "DATABASE2: %s\n", db_name.c_str());
                db = dbmap_find(iProxyId, db_name, "pgsql");
            } 
            index = request_in.FindSubString("options");
            if(index != std::string::npos)
            {
                index += (unsigned)strlen("options");
                db_options.append((const char*)data + index + 1);
                logevent(SQL_DEBUG, "COMMAND LINE PARAMETERS: %s\n", db_options.c_str());
            }
        }
        first_request = false;
        // we do not have any additional commands in first packet.
        // just send everything we got so far.
        request_in.pop(request, full_size);
        return true;
    }

    // we got command? 
    switch(request_type)
    {
    case 0:
        {
            if(data[4] == 16)
                logevent(SQL_DEBUG, "PGSQL CANCEL REQUEST command\n");

        }
    case PGSQL_QUERY:
        {
            if(data[1] != 0)
                break;
            unsigned int request_size = data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
            if(request_size + 1 > full_size)
                return true;
            longResponse = true;
            longResponseData = false;
            query.append((char *)data + 5);
            logevent(SQL_DEBUG, "QUERY command[%s]: %s\n", 
                 db_name.c_str(), query.c_str());

            if ( check_query(query) == false)
            {
                str_lowercase(query);
                loghex(NET_DEBUG, (unsigned char*)query.c_str(), (int)query.size());
                // bad query - block it
                request = ""; // do not send it to backend server
                blockResponse(response);
                
                if (response_in.size() != 0)
                {
                    // push it to the server response parsing queue
                    response_in.append(response.c_str(), (int)response.size());
                } else {
                    // push it to the client
                    response_out.append(response.c_str(), (int)response.size());
                }
                hasResponse = true;
                request_in.pop(request, request_size + 1);
                request = "";
                return true;
            } 
        }break;
    case PGSQL_QUIT: 
        logevent(SQL_DEBUG, "PGSQL QUIT command\n");break;
    case PGSQL_BIND: 
        {
            if(data[1] != 0)
                break;
            logevent(SQL_DEBUG, "PGSQL BIND command\n");
            prepareName.append((char *)data + 5);
            if(!prepareName.empty())
                logevent(SQL_DEBUG, "destination portal name: %s\n",prepareName.c_str());
            query.append((char *)data + strlen(prepareName.c_str())+6);
            logevent(SQL_DEBUG, "Prepared statement name: %s\n",query.c_str());
            if(FindErrorInParse(query))
            {
                blockParseResponse(response);
                ClearParseError(query);
                if (response_in.size() != 0)
                {
                    // push it to the server response parsing queue
                    response_in.append(response.c_str(), (int)response.size());
                } 
                else 
                {
                    // push it to the client
                    response_out.append(response.c_str(), (int)response.size());
                }
                hasResponse = true;
                request_in.pop(request, full_size);
                request = "";
                return true;
            }
//             int ind = query.size() + 6,parameterNum;
//             parameterNum = data[ind] << 8 | data[ind + 1];
//             std::string parameterCodes;
//             parameterCodes.append(data + ind + 2,parameterNum * 2);
        }
        break;
    case PGSQL_CLOSE: 
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL CLOSE command\n");break;
    case PGSQL_COPY_DATA: 
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COPY DATA backend command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_EXECUTE: 
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL EXECUTE command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_FLUSH: 
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL FLUSH command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_FUNC_CALL: 
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL FUNCTION CALL command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_PARSE: 
        {
            if(data[1] != 0)
                break;
            logevent(SQL_DEBUG, "PGSQL PARSE command\n");
            prepareName.append((char *)data + 5);
            logevent(SQL_DEBUG, "Prepared statement name: %s\n",prepareName.c_str());
            query.append((char *)data + strlen(prepareName.c_str())+6);
            logevent(SQL_DEBUG, "Prepared query name: %s\n",query.c_str());
            if ( check_query(query) == false)
            {
                SetParseError(prepareName);
                blockParseResponse(response);
                if (response_in.size() != 0)
                {
                    // push it to the server response parsing queue
                    response_in.append(response.c_str(), (int)response.size());
                } 
                else 
                {
                    // push it to the client
                    response_out.append(response.c_str(), (int)response.size());
                }
                hasResponse = true;
                request_in.pop(request, full_size);
                request = "";
                return true;
            }
        }
        break;
    case PGSQL_SYNC:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL SYNC command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_COPY_DONE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL COPY DONE backend command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_COPY_FAIL:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COPY FAIL command\n");loghex(SQL_DEBUG, data, full_size);break;
    case PGSQL_DESCRIBE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL DESCRIBE command\n");loghex(SQL_DEBUG, data, full_size);break;
    default:
        if(data[1] == 0)
        {
            logevent(SQL_DEBUG, "UNKNOWN COMMAND\n");
            loghex(SQL_DEBUG, data, full_size);
        }
         
    } 
    request_in.pop(request, full_size);
    if (hasResponse == true)
        request = "";

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
    unsigned int max_response_size = (unsigned int) response_in.size();

    if (max_response_size < 3)
    {
       logevent(NET_DEBUG, "received %d bytes of response\n", max_response_size);
       response_in.pop(response,response_in.size());
       return true;
    }
    const unsigned char * data = response_in.raw();
    //////////////////////////////////////////////////////////////////////////
    //debug
    loghex(SQL_DEBUG, data, max_response_size);
    //////////////////////////////////////////////////////////////////////////

    // max packet size is 16 MB
    unsigned int response_size = data[4];
    unsigned int type = data[0];

    logevent(NET_DEBUG, "packet size expected %d bytes (received %d)\n", 
            response_size, max_response_size);

    // could happen when using "SET command"
    switch(type)
    {
    case PGSQL_SRV_PASSWORD_MESSAGE:
        {
            unsigned int auth = data[4];
            type = data[8];
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
                        logevent(NET_DEBUG, "PGSQL Authentication Ok\n");
                        unsigned int type = data[9];
                        if(type == 83)
                        {
                            unsigned int ind = response_in.FindSubString("server_version");
                            if(ind >0)
                            {
                                db_srv_version.append((const char*)data + ind + strlen("server_version")+1,strlen((const char*)data + ind+strlen("server_version")+1));
                            }
                        }
						if(data[9] == PGSQL_EXECUTE)
							ErrorResponseHandling(data,15);
                        response_in.pop(response, max_response_size);
                        longResponse = false;
                        longResponseData = false;
                        return true;
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
    case PGSQL_BECKEND_KEY_DATA: 
        if(data[4] == 12)
            logevent(SQL_DEBUG, "PGSQL BECKEND KEY DATA command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_BIND_COMPLETE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL BIND COMPLETE command\n");break;
    case PGSQL_CLOSE_COMPLETE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL CLOSE COMPLETE command\n");break;
    case PGSQL_CLOSE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COMMAND COMPLETE\n");break;
    case PGSQL_COPY_DATA: 
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COPY DATA frontend command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_COPY_DONE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL COPY DONE frontend command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_COPY_IN_RESPONSE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COPY IN RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_FLUSH:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL COPY OUT RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_EMPTY_QUERY_RESPONSE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL EMPTY QUERY RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_EXECUTE:
        if(data[1] == 0)
			ErrorResponseHandling(data);
		break;
    case PGSQL_FUN_CALL_RESPONSE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL FUNCTION CALL RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_NO_DATA:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL NO DATA command\n");break;
    case PGSQL_NOTICE_RESPONSE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL NOTICE RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_NOTIF_RESPONSE:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL NOTIFICATION RESPONSE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_SYNC:
        if(data[1] == 0)
            logevent(SQL_DEBUG, "PGSQL PARAMETER STATUS command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_PARSE_COMPLETE:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL PARSE COMPLETE command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    case PGSQL_PORTAL_SUSPENDED:
        if(data[4] == 4)
            logevent(SQL_DEBUG, "PGSQL PORTAL SUSPENDED command\n");loghex(SQL_DEBUG, data, max_response_size);break;
    }
    response_in.pop(response, max_response_size);
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
