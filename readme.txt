
The following directory contains source code of the GreenSQL firewall.
GreenSQL is an Open Source database firewall used to protect databases
from SQL injection attacks. GreenSQL works as a proxy and has built in
support for MySQL. The logic is based on evaluation of SQL commands 
using a risk scoring matrix as well as blocking known db administrative 
commands (DROP, CREATE, etc).

For additional info check:

http://www.greensql.net/

In order to run this application you need the following packages:

libpcre
libmysqlclient
libevent

