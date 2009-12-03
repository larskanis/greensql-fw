
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
libpq (for postgresql)
 
You can install the various dependencies on debian by running:

apt-get install libpcre3
apt-get install libmysqlclient15off
apt-get install libevent1
apt-get install libpq5 

You can install the various dependencies on centos by running:

yum install libevent
yum install mysql
yum install pcre
yum install postgresql-libs

