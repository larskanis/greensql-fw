#!/bin/sh

MYSQL_ADMIN_USER="root"
MYSQL_ADMIN_PWD=""
MYSQL_HOST="127.0.0.1"
MYSQL_HOSTNAME=""
MYSQL_PORT=""
GREENSQL_DB_USER="green"
GREENSQL_DB_PWD="pwd"
GREENSQL_DB_NAME="greendb"
GREENSQL_CONFIG_FILE="/etc/greensql/greensql.conf"
MY_CNF=""
MRO=""

change_conf()
{
  echo -n "MySQL admin user [$MYSQL_ADMIN_USER]: "
  read cont
  if [ "$cont" != "" ]; then
    MYSQL_ADMIN_USER=$cont
  fi

  echo -n "MySQL admin password [$MYSQL_ADMIN_PWD]: "
  read cont
  if [ "$cont" != "" ]; then
    MYSQL_ADMIN_PWD=$cont
  fi

  echo -n "MySQL server address (you can use ip:port string) [$MYSQL_HOST]: "
  read cont
  if [ "$cont" != "" ]; then
    MYSQL_HOST=$cont
  fi

  echo -n "GreenSQL config db name [$GREENSQL_DB_NAME]: "
  read cont
  if [ "$cont" != "" ]; then
    GREENSQL_DB_NAME=$cont
  fi

  echo -n "GreenSQL DB user name [$GREENSQL_DB_USER]: "
  read cont
  if [ "$cont" != "" ]; then
    GREENSQL_DB_USER=$cont
  fi

  echo -n "GreenSQL DB user password [$GREENSQL_DB_PWD]: "
  read cont
  if [ "$cont" != "" ]; then
    GREENSQL_DB_PWD=$cont
  fi

  main_fn
#  create_mysql_config
#  create_db
#  clean_mysql_config
#  exit
}

change_pwd()
{
  echo -n "Use the following password [$GREENSQL_DB_PWD]: "
  read cont
  if [ "$cont" != "" ]; then  
    GREENSQL_DB_PWD=$cont
  fi
}

print_settings()
{
  echo ""
  echo "MySQL admin user: [$MYSQL_ADMIN_USER]"
  echo "MySQL admin password: [$MYSQL_ADMIN_PWD]"
  echo "MySQL server address: [$MYSQL_HOST]"
  echo ""
  echo "GreenSQL configuration DB name: [$GREENSQL_DB_NAME]"
  echo "DB user to create: [$GREENSQL_DB_USER]"
  echo "Password to set: [$GREENSQL_DB_PWD]"
  echo ""
}


main_fn()
{
  echo ""
  echo "---------------------------------------------"
  echo "The following settings will be used:"
  print_settings
  echo -n "Do you want to change anything? [y/N] "
  read cont
  echo ""

  if [ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" = "Y" ]; then
    change_conf
    echo ""
  fi

}

create_mysql_config()
{
  # Create a custom temporary MySQL configuration file for the root user.
  MY_CNF=`mktemp /tmp/greensql.my-cnf.XXXXXXXXXX`
  chmod 0600 $MY_CNF

  # Split MYSQL_HOST for the host:port pair, give an empty string for the
  # port if it isn't specified, save it into MYSQL_PORT otherwise.
  mysql_h="`echo $MYSQL_HOST|cut -d: -f1`"
  mysql_p="`echo $MYSQL_HOST|cut -d: -f2`"
  if [ "$mysql_h" != "$mysql_p" -a "x$mysql_p" != "x" ]; then
    MYSQL_HOSTNAME="$mysql_h"
    MYSQL_PORT="$mysql_p"
  else
    MYSQL_HOSTNAME="$MYSQL_HOST"
    MYSQL_PORT=""
  fi

  echo "[client]
        host=${MYSQL_HOST}
        port=${MYSQL_PORT}
        user=${MYSQL_ADMIN_USER}" > $MY_CNF
  if [ "${MYSQL_ADMIN_PWD}" != "none" ]; then
    echo "password=${MYSQL_ADMIN_PWD}" >> $MY_CNF
  fi
  MRO="--defaults-file=$MY_CNF"
}

clean_mysql_config()
{
  if [ "$MY_CNF" != "" ]; then
    rm -rf $MY_CNF
  fi
}

create_db()
{
  if mysql $MRO -BNe 'show databases' | grep -q "$GREENSQL_DB_NAME"; then
    echo "Database already exists, doing nothing."
  else
    echo "Creating MySQL database..."
    mysqladmin $MRO -f create $GREENSQL_DB_NAME > /dev/null
    echo "Adding MySQL user..."
    if [ "$MYSQL_HOSTNAME" = "localhost" -o "$MYSQL_HOSTNAME" = "127.0.0.1" ]
    then
      mysql $MRO $GREENSQL_DB_NAME -f -e "GRANT ALL ON $GREENSQL_DB_NAME.* TO '$GREENSQL_DB_USER'@'localhost' IDENTIFIED BY '${GREENSQL_DB_PWD}'" > /dev/null
    else 
      mysql $MRO $GREENSQL_DB_NAME -f -e "GRANT ALL ON $GREENSQL_DB_NAME.* TO '$GREENSQL_DB_USER'@'%' IDENTIFIED BY '${GREENSQL_DB_PWD}'"
    fi
    create_tables
  fi

}

create_tables()
{
echo "Creating MySQL tables..."
mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table query
(
queryid int unsigned NOT NULL auto_increment primary key,
proxyid        int unsigned NOT NULL default '0',
perm           smallint unsigned NOT NULL default 1,
db_name        char(50) NOT NULL,
query          text NOT NULL,
INDEX(proxyid,db_name)
);" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table proxy
(
proxyid        int unsigned NOT NULL auto_increment primary key,
proxyname      char(50) NOT NULL default '',
frontend_ip    int unsigned NOT NULL default 0,
frontend_port  smallint unsigned NOT NULL default 0,
backend_server char(50) NOT NULL default '',
backend_ip     int unsigned NOT NULL default 0,
backend_port   smallint unsigned NOT NULL default 0,
dbtype         char(20) NOT NULL default 'mysql',
status         smallint unsigned NOT NULL default '1'
);" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"insert into proxy values (0,'Default Proxy',INET_ATON('127.0.0.1'),3305,
'localhost',INET_ATON('127.0.0.1'),3306,'mysql',1);" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table db_perm
(
dbpid          int unsigned NOT NULL auto_increment primary key,
proxyid        int unsigned NOT NULL default '0',
db_name        char(50) NOT NULL,
create_perm    smallint unsigned NOT NULL default 0,
drop_perm      smallint unsigned NOT NULL default 0,
alter_perm     smallint unsigned NOT NULL default 0,
info_perm      smallint unsigned NOT NULL default 0,
block_q_perm   smallint unsigned NOT NULL default 0,
INDEX (proxyid, db_name)
);" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table user
(
userid         int unsigned NOT NULL auto_increment primary key,
name           char(50) NOT NULL default '',
pwd            char(50) NOT NULL default '',
email          char(50) NOT NULL default ''
);" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"insert into user values(0,'admin',sha1('pwd'),'');" > /dev/null

mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table alert
(
alertid             int unsigned NOT NULL auto_increment primary key,
agroupid            int unsigned NOT NULL default '0',
event_time          datetime NOT NULL,
risk                smallint unsigned NOT NULL default '0',
block               smallint unsigned NOT NULL default '0',
user                varchar(50) NULL default '',
query               text NOT NULL,
reason              text NOT NULL
);" > /dev/null


mysql $MRO $GREENSQL_DB_NAME -e \
"CREATE table alert_group
(
agroupid            int unsigned NOT NULL auto_increment primary key,
proxyid             int unsigned NOT NULL default '1',
db_name             char(50) NOT NULL default '',
update_time         datetime NOT NULL,
status              smallint NOT NULL default 0,
pattern             text NOT NULL,
INDEX(update_time)
);" > /dev/null

}

# next function s used to update greensqll config file
update_greensql_config()
{
# check if the file is writable
if [ ! -w $GREENSQL_CONFIG_FILE ]; then
  echo ""
  echo "GreenSQL configuration file is not writable!!!"
  echo "Check that [database] section contains the following settings in"
  echo "$GREENSQL_CONFIG_FILE" 
  echo ""
  echo "[database]"
  echo "dbhost=$MYSQL_HOSTNAME"
  echo "dbname=$GREENSQL_DB_NAME"
  echo "dbuser=$GREENSQL_DB_USER"
  echo "dbpass=$GREENSQL_DB_PWD"
  if [ "$MYSQL_PORT" != "" ] && [ "$MYSQL_PORT" != "3306" ]; then
    echo "dbport=${MYSQL_PORT}"
  else
    echo "# dbport=3306"
  fi
  echo ""
  return
fi

echo "Modifing $GREENSQL_CONFIG_FILE..."

# save start and end of the config file
start_cfg=`perl -p0777 -e 's/\[database\].*$//s' $GREENSQL_CONFIG_FILE`
end_cfg=`perl -p0777 -e 's/^.*\[database\][^\[]*\[/\[/s' $GREENSQL_CONFIG_FILE`

  echo \
"$start_cfg

[database]
dbhost=$MYSQL_HOSTNAME
dbname=$GREENSQL_DB_NAME
dbuser=$GREENSQL_DB_USER
dbpass=$GREENSQL_DB_PWD" > $GREENSQL_CONFIG_FILE

  if [ "$MYSQL_PORT" != "" ] && [ "$MYSQL_PORT" != "3306" ]; then
    echo "dbport=${MYSQL_PORT}" >> $GREENSQL_CONFIG_FILE
  else
    echo "# dbport=3306" >> $GREENSQL_CONFIG_FILE
  fi
echo \
"
$end_cfg" >> $GREENSQL_CONFIG_FILE
}

# execution of the script start here:
main_fn
if [ "$GREENSQL_DB_PWD" == "pwd" ]; then
  echo "The following password for [$GREENSQL_DB_USER] user will be set: [$GREENSQL_DB_PWD]"
  echo ""
  echo -n "Do you want to change default password? [Y/n] "
  read cont
  echo ""
  if [ "$cont" == "" ] || [ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" = "Y" ]; then
    change_pwd
  fi

fi

create_mysql_config
create_db
clean_mysql_config
update_greensql_config

