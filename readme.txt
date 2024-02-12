Overview

GreenSQL is an Open Source database firewall used to protect databases
from SQL injection attacks. GreenSQL works as a proxy and has built in
support for MySQL. The logic is based on evaluation of SQL commands 
using a risk scoring matrix as well as blocking known db administrative 
commands (DROP, CREATE, etc).

Compilation

In order to build the debian package you will need to install the folowing:

sudo apt-get install gcc g++
sudo apt-get install make
sudo apt-get install devscripts
sudo apt-get install fakeroot
sudo apt-get install debhelper
sudo apt-get install build-essential
sudo apt-get install flex
sudo apt-get install bison
sudo apt-get install libevent-dev
sudo apt-get install libmysqlclient15-dev
sudo apt-get install libpcre3
sudo apt-get install libpcre3-dev
sudo apt-get install libpq-dev

For OpenSuse you need to run the following commands that will install missing packages:

yast -i flex bison gcc gcc-c++ make
yast -i postgresql-devel libmysqlclient-devel
yast -i pcre-devel libevent-devel

If you use default installation of FreeBSD you will need to install the following packages:

pkg_add -r bison
pkg_add -r libevent
pkg_add -r pcre
pkg_add -r mysql51-client

Installation

Before proceeding with the instalation you need to copy the source
code of the greensql firewall. You can do this by cloning this repository 
to your local machine using git clone.

Enter your cloned directory and run the following command:

  ./build.sh

After this there are two ways to go about setting up the firewall,
either through manual installation or building your own package. 

Building your own installation package
--------------------------------------
run the following shell script:

  greensql-create-db.sh

Next step is to start the application, run the following command as a root user:

  /etc/init.d/greensql-fw start


Manual Installation
-------------------
There are a number of steps to perform in order to install application.
They are:

1. Create dedicated user for greensql service.
2. Creating MySQL config db and a db user.
3. Setting up configuration files.
3. Setting log file.
4. Configure start up scripts


Creating greensql system group and user
---------------------------------------

In order to create greensql group and user run the following
commands (run these commands as a root user):

   groupadd greensql
   useradd -M -g greensql -s /dev/null greensql

Alternatively you do the ame by execurting the following commands:
  
   cd scripts/
   ./setup_user.sh


Creating MySQL DB and user
--------------------------
Just run the following script: greensql-create-db.sh . It will
automatically create configuration database.

   cd scripts/
   ./greensql-create-db.sh

Setting up configuration files
------------------------------

You will find a number of configuration files in the ./conf/ directory.
GreenSQL start up script expects to find the configuration files in the
following directory:

  /etc/greensql/

You simply need to copy files from ./conf/* to /etc/greensql . You can do it
as followed:

   mkdir -p /etc/greensql
   cp ./conf/* /etc/greensql/ -r
   chown greensql:greensql /etc/greensql -R
   chmod 700 /etc/greensql

Alternativly you can run setup_conf.sh file located in the scripts 
directory.

   cd scripts/
   ./setup_conf.sh

Next step is to alter /etc/greensql/greensql.conf file and specify correct 
db name, server, port, user and password.


Setting log file
----------------

By default greensql expects to find log file in:

/var/log/greensql.log

In addition, log file rotation must be enabled. You can do it by running
the following commands:

   touch /var/log/greensql.log
   chown greensql:greensql /var/log/greensql.log
   chmod 600 /var/log/greensql.log
   cp scripts/greensql.rotate /etc/logrotate.d/greensql

Alternatively you can run ./setup_log.sh script located in the scripts/
directory.


Configure start up scripts
--------------------------

As a final step you need to copy greensql-fw binary to the /user/sbin/
or /sbin directory and copy greensql service initialization script to the 
/etc/init.d/ directory.

Run the following commands:
 
   cp greensql-fw /usr/sbin/
   cp scripts/rc.greensql /etc/init.d/greensql

Another alternative is to run ./setup_binary.sh script located in the scripts/
directory.

Start the application:

   /etc/init.d/greensql start

Stop the application:

   /etc/init.d/greensql stop

Troubleshooting

If you encounter problems at any point run:

   tail -f /var/log/greensql.log

License

GNU General Public License v2.0

