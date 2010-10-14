#!/bin/sh

#
# Copyright (c) 2007 GreenSQL.NET <stremovsky@gmail.com>
# License: GPL v2 (http://www.gnu.org/licenses/gpl.html)
#

VER=1.3.0
SQL="mysql"
GREENSQL_DB_USER="green"
GREENSQL_DB_PWD="pwd"
GREENSQL_DB_NAME="greendb"
CONF="/etc/greensql/greensql.conf"
CONFWEB="/usr/share/greensql-fw/config.php"
MY_CNF=""
MRO=""

ZCAT=`which zcat 2>/dev/null` || true
GUNZIP=`which gunzip 2>/dev/null` || true

PSQL=`which psql`
if [ -z $PSQL ] && [ -d "/opt/PostgreSQL/" ]; then
  PSQL=`find /opt/PostgreSQL/*/bin -name psql | grep psql -m 1`
fi

MYSQL=`which mysql`
MYSQLADMIN=`which mysqladmin`

echo " "

if [ ! -z $MYSQL ]; then
  echo "mysql binary file located: $MYSQL"
fi

if [ ! -z $MYSQLADMIN ]; then
  echo "mysqladmin binary file located: $MYSQLADMIN"
fi

if [ ! -z $PSQL ]; then
  echo "postgresql binary file located: $PSQL"
fi

if [ -d /usr/share/doc/greensql-fw-$VER/ ]; then
  DOCDIR="/usr/share/doc/greensql-fw-$VER"
elif [ -d /usr/share/doc/greensql-fw/ ]; then
  DOCDIR="/usr/share/doc/greensql-fw"
elif [ -d /usr/share/doc/packages/greensql-fw/ ]; then
  DOCDIR="/usr/share/doc/packages/greensql-fw/"
else
  echo "No Doc Dir Found."
  echo "Probably greensql-fw is not installed"
  echo ""
  exit 1;
fi

###############################
## PgSQL Authentication Type ##
###############################

data_dir=`ps ax | grep "postgres" | grep "\-D" | grep -iv grep | perl -e 'if (<STDIN> =~ m/\-D\s*(.*?)\s/) {print $1;}'`
if [ -z "$data_dir" ]; then
  data_dir=`ps ax | grep "postmaster" | grep "\-D" | grep -iv grep | perl -e 'if (<STDIN> =~ m/\-D\s*(.*?)\s/) {print $1;}'`
fi

if [ ! -z "$data_dir" ]; then
  pgsql_conf=`ps ax | grep "postgres" | grep "\-D" | grep -iv grep | perl -e 'if (<STDIN> =~ m/config_file\s*=\s*(.*?)\s/) {print $1}'`

  if [ -z $pgsql_conf ]; then
    pgsql_conf="${data_dir}/postgresql.conf"
  fi

  ## get hba_file from configuration file
  hba_file=`grep -E "^\s*hba_file" $pgsql_conf`
  if [ ! -z "$hba_file" ]; then
    hba_file=`echo $hba_file | tr -d "'"`
    hba_file=`echo $hba_file | perl -e 'if (<STDIN> =~ m/hba_file\s*=\s*(.*?)\s/) { print $1;}'`
  else
    ## couldnt find hba_file directive in configuration so try the data dir
    hba_file="${data_dir}/pg_hba.conf"
  fi

  if [ ! -f $hba_file ]; then
    echo "file doesnt exist: $hba_file"
  elif [ ! -r $hba_file ]; then
    echo "Failed to read file: $hba_file"
  fi

  rule=`grep -E "^\W*local\W*all\W*all\W*ident" $hba_file` || true
  if [ ! -z "$rule" ]; then
    POSTGRES_AUTH="ident"
  else
    rule=`grep -E "^\W*local\W*all\W*all\W*md5" $hba_file` || true
    if [ ! -z "$rule" ]; then
      POSTGRES_AUTH="md5"
    fi
  fi
fi

main_fn()
{
  echo " "
  echo -n "Database type (mysql or pgsql) [$SQL]: "
  read cont
  if [ "$cont" != "" ]; then
    SQL=$cont
  fi

  if [ "$SQL" = "mysql" ]; then
    mysql_binaries
    mysql_config
  elif [ "$SQL" = "pgsql" ]; then
    pgsql_binaries
    pgsql_config
  else
    echo "no valid database type was chosen"
    exit 0;
  fi
}

## check if mysql binaries are present were found
mysql_binaries()
{
  while [ "$MYSQL" = "" ]; do
    echo -n "Path to MySQL binary file: "
    read cont
    if [ -f "$cont" ]; then
      if [ -x "$cont" ]; then
        if file $cont | grep ELF >> /dev/null 2>&1; then
          MYSQL=$cont
        else
          echo "File is not a binary: $cont"
        fi
      else
        echo "File is not executable: $cont"
      fi
    else
      if [ -d "$cont" ]; then
        echo "$cont is a directory"
      else
        echo "File not found: $cont"
      fi
    fi
  done

  while [ "$MYSQLADMIN" = "" ]; do
    echo -n "Path to MySQL Administration binary file: "
    read cont
    if [ -f "$cont" ]; then
      if [ -x "$cont" ]; then
        if file $cont | grep ELF >> /dev/null 2>&1; then
          MYSQLADMIN=$cont
        else
          echo "File is not a binary: $cont"
        fi
      else
        echo "File is not executable: $cont"
      fi
    else
      if [ -d "$cont" ]; then
        echo "$cont is a directory"
      else
        echo "File not found: $cont"
      fi
    fi
  done
}

pgsql_binaries()
{
  while [ "$PSQL" = "" ]; do 
    echo -n "Path to PgSQL binary file: "
    read cont
    if [ -f "$cont" ]; then
      if [ -x "$cont" ]; then
        if file $cont | grep ELF >> /dev/null 2>&1; then
          PSQL=$cont
        else
          echo "File is not a binary: $cont"
        fi
      else
        echo "File is not executable: $cont"
      fi
    else
      if [ -d "$cont" ]; then
        echo "$cont is a directory"
      else
        echo "File not found: $cont"
      fi
    fi
  done
}

## mysql configuration function 
mysql_config()
{
  MYSQL_HOST="127.0.0.1"
  MYSQL_PORT="3306"
  MYSQL_ADMIN_USER="root"
  MYSQL_ADMIN_PWD=""

  echo -n "MySQL server address [$MYSQL_HOST]: "
  read cont
  if [ "$cont" != "" ]; then
    MYSQL_HOST=$cont
  fi

  echo -n "MySQL port number [$MYSQL_PORT]: "
  read cont
  if [ "$cont" != "" ]; then
    MYSQL_PORT=$cont
  fi

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
  
  ## file for automatically connecting to server through script
  create_user_questions
  echo -n "Would you like to set up the database and tables automatically [Y/n]: "
  read cont
  if [ "$cont" = "n" ] || [ "$cont" = "N" ]; then
    echo " "
  else
    echo $cont
    create_mysql_config
    create_mysql_db
    clean_mysql_config
  fi
}

## main postgresql configuration function
pgsql_config()
{
  POSTGRES_LOCATION="local"

  echo -n "location database or remote database (local or remote) [$POSTGRES_LOCATION]: "
  read cont
  if [ "$cont" != "" ]; then
    POSTGRES_LOCATION=$cont
  fi

  if [ "$POSTGRES_LOCATION" = "local" ]; then
    if [ -z $POSTGRES_AUTH ]; then
      echo -n "PostgreSQL Authentication Type (ident or md5) [ident]: "
      read cont
      if [ "$cont" = "ident" ]; then
        POSTGRES_AUTH="ident"
      elif [ "$cont" = "md5" ]; then
        POSTGRES_AUTH="md5"
      else
        POSTGRES_AUTH="ident"
      fi
    fi
  elif [ "$POSTGRES_LOCATION" = "remote" ]; then
    pgsql_config_remote
    POSTGRES_AUTH="md5"
  fi

  if [ "$POSTGRES_AUTH" = "md5" ]; then
    pgsql_config_credentials
  fi

  create_user_questions

  echo -n "Would you like to set up the database and tables automatically[Y/n]: "
  read cont
  if [ "$cont" = "n" ] || [ "$cont" = "N" ]; then
    echo " "
  else
    echo $cont
    create_pgsql_config
    create_pgsql_db
    clean_pgsql_config
  fi
}

create_pgsql_config()
{
  MY_PGPASS=~/.pgpass

  if [ -f $MY_PGPASS ]; then
    mv $MY_PGPASS ~/.pgpass-temp
  fi

  touch $MY_PGPASS
  chmod 0600 $MY_PGPASS

  if [ $POSTGRES_LOCATION = "local" ]; then
    ## administrator rules already working on localhost automatically due to ident
    if [ "$POSTGRES_AUTH" = "md5" ]; then
      echo "127.0.0.1:*:postgres:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
      echo "localhost:*:postgres:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
      echo "127.0.0.1:*:$GREENSQL_DB_NAME:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
      echo "localhost:*:$GREENSQL_DB_NAME:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
    fi

    ## new user rule - md5 anyway since it's not a system user
    echo "localhost:*:$GREENSQL_DB_NAME:$GREENSQL_DB_USER:$GREENSQL_DB_PWD" >> $MY_PGPASS
    echo "127.0.0.1:*:$GREENSQL_DB_NAME:$GREENSQL_DB_USER:$GREENSQL_DB_PWD" >> $MY_PGPASS
  elif [ $POSTGRES_LOCATION = "remote" ]; then
    ## administrator rules
    echo "$POSTGRES_HOST:$POSTGRES_PORT:postgres:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
    echo "$POSTGRES_HOST:$POSTGRES_PORT:$GREENSQL_DB_NAME:$POSTGRES_ADMIN_USER:$POSTGRES_ADMIN_PWD" >> $MY_PGPASS
    ## user rule
    echo "$POSTGRES_HOST:$POSTGRES_PORT:$GREENSQL_DB_NAME:$GREENSQL_DB_USER:$GREENSQL_DB_PWD" >> $MY_PGPASS
  fi
}

## postgresql configuration function for REMOTE servers
pgsql_config_remote()
{
  POSTGRES_HOST="127.0.0.1"
  POSTGRES_PORT="5432"

  echo -n "PostgreSQL server address [$POSTGRES_HOST]: "
  read cont
  if [ "$cont" != "" ]; then
    POSTGRES_HOST=$cont
  fi

  echo -n "PostgreSQL port number [$POSTGRES_PORT]: "
  read cont
  if [ "$cont" != "" ]; then
    POSTGRES_POST=$cont
  fi
}

pgsql_config_credentials()
{
  POSTGRES_ADMIN_USER="postgres"

  echo -n "PostgreSQL admin user [$POSTGRES_ADMIN_USER]: "
  read cont
  if [ "$cont" != "" ]; then
    POSTGRES_ADMIN_USER=$cont
  fi

  echo -n "PostgreSQL admin password [$POSTGRES_ADMIN_PWD]: "
  read cont
  if [ "$cont" != "" ]; then
    POSTGRES_ADMIN_PWD=$cont
  fi
}

create_mysql_config()
{
  # Create a custom temporary MySQL configuration file for the root user.
  MY_CNF=`mktemp /tmp/greensql.my-cnf.XXXXXXXXXX`
  chmod 0600 $MY_CNF

  echo "[client]
        host=${MYSQL_HOST}
        port=${MYSQL_PORT}
        user=${MYSQL_ADMIN_USER}" > $MY_CNF
  if [ "${MYSQL_ADMIN_PWD}" != "" ]; then
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

clean_pgsql_config()
{
  if [ -f ~/.pgpass-temp ]; then
    mv ~/.pgpass-temp ~/.pgpass
  fi
}

create_pgsql_db()
{
  SKIP_DB=0
  EXISTS_DB=0

  if [ "$POSTGRES_LOCATION" = "local" ]; then
    if [ "$POSTGRES_AUTH" = "ident" ]; then
      if su - postgres -c "$PSQL -c \"select * from pg_database where datname='$GREENSQL_DB_NAME';\" | grep -q \"$GREENSQL_DB_NAME\""; then
        SKIP_DB=1
        EXISTS_DB=1
        echo " "
        echo "database $GREENSQL_DB_NAME already exists."
        echo -n "Do you want to drop and recreate database $GREENSQL_DB_NAME [N/y]: "
        read cont
        if [ "$cont" = "Y" ] || [ "$cont" = "y" ]; then
          SKIP_DB=0
        else
          SKIP_DB=1
          echo "Skipping database and user creation"
        fi
      fi

      if [ "$SKIP_DB" = "0" ]; then
        if [ "$EXISTS_DB" = "1" ]; then
          if ! su - postgres -c "$PSQL -c \"DROP DATABASE $GREENSQL_DB_NAME\""; then
            echo "Failed to drop database $GREENSQL_DB_NAME"
            echo "Aborting.."
            exit 0
          fi
        fi

        ## creating user
        create_user_pgsql

        echo "Adding database $GREENSQL_DB_NAME..."
        if ! su - postgres -c "$PSQL -c \"CREATE DATABASE $GREENSQL_DB_NAME OWNER $GREENSQL_DB_USER\""; then
          echo "Failed to create database $GREENSQL_DB_NAME"
          echo "Aborting.."
          exit 0
        else 
          create_tables_pgsql
        fi
      fi
    elif [ "$POSTGRES_AUTH" = "md5" ]; then
      if $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "select * from pg_database where datname='$GREENSQL_DB_NAME';" | grep -q "$GREENSQL_DB_NAME"; then
        SKIP_DB=1
        EXISTS_DB=1
        echo " "
        echo "database $GREENSQL_DB_NAME already exists."
        echo -n "Do you want to drop and recreate database $GREENSQL_DB_NAME [N/y]: "
        read cont
        if [ "$cont" = "Y" ] || [ "$cont" = "y" ]; then
          SKIP_DB=0
        else
          SKIP_DB=1
          echo "Skipping database and user creation"
        fi
      fi

      if [ "$SKIP_DB" = "0" ]; then
        if [ "$EXISTS_DB" = "1" ]; then
          echo "Dropping database $GREENSQL_DB_NAME..."
          if ! $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "DROP DATABASE $GREENSQL_DB_NAME"; then
            echo "Failed to drop database $GREENSQL_DB_NAME"
            echo "Aborting.."
            exit 0
          fi
        fi

        ## creating user
        create_user_pgsql

        echo "Adding database $GREENSQL_DB_NAME..."
        if ! $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "CREATE DATABASE $GREENSQL_DB_NAME OWNER $GREENSQL_DB_USER"; then
          echo "Failed to create database $GREENSQL_DB_NAME"
          echo "Aborting.."
          exit 0
        else
          create_tables_pgsql
        fi
      fi
    fi
  elif [ "$POSTGRES_LOCATION" = "remote" ]
  then
    if $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "select * from pg_database where datname='$GREENSQL_DB_NAME';"| grep -q "$GREENSQL_DB_NAME"
    then
        SKIP_DB=1
        EXISTS_DB=1
        echo " "
        echo "database $GREENSQL_DB_NAME already exists."
        echo -n "Do you want to drop and recreate database $GREENSQL_DB_NAME [N/y]: "
        read cont
        if [ "$cont" = "Y" ] || [ "$cont" = "y" ]; then
          SKIP_DB=0
        else
          SKIP_DB=1
          echo "Skipping database and user creation"
        fi
    fi

    if [ "$SKIP_DB" = "0" ]; then
      if [ "$EXISTS_DB" = "1" ]; then
        echo "Dropping database $GREENSQL_DB_NAME..."
        if ! $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "CREATE DATABASE $GREENSQL_DB_NAME OWNER $GREENSQL_DB_USER"; then
          echo "Failed to drop database $GREENSQL_DB_NAME"
          echo "Aborting.."
          exit 0
        fi
      fi

      ## creating user
      create_user_pgsql

      echo "Adding database $GREENSQL_DB_NAME..."
      if ! $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "CREATE DATABASE $GREENSQL_DB_NAME OWNER $GREENSQL_DB_USER"; then
        echo "Failed to create database $GREENSQL_DB_NAME"
        echo "Aborting.."
        exit 0
      else
        create_tables_pgsql
      fi
    fi
  fi
}

create_mysql_db()
{
  SKIP_DB=0
  EXISTS_DB=0

  if $MYSQL $MRO -BNe 'show databases' | grep -q "$GREENSQL_DB_NAME"; then
    EXISTS_DB=1
    echo " "
    echo "database $GREENSQL_DB_NAME already exists."
    echo -n "Do you want to drop and recreate database $GREENSQL_DB_NAME [N/y]: "
    read cont
    if [ "$cont" = "Y" ] || [ "$cont" = "y" ]; then
      SKIP_DB=0
    else
      SKIP_DB=1
      echo "Skipping database and user creation"
    fi
  fi

  if [ "$SKIP_DB" = "0" ]; then
    if [ "$EXISTS_DB" = "1" ]; then
      echo "dropping MySQL database $GREENSQL_DB_NAME..."
      if ! $MYSQLADMIN $MRO -f drop $GREENSQL_DB_NAME; then
        echo "Failed to drop database $GREENSQL_DB_NAME"
        echo "Aborting.."
        exit 0
      fi
    fi

    echo "Creating MySQL database..."
    if ! $MYSQLADMIN $MRO -f create $GREENSQL_DB_NAME; then
      echo "Failed to create database $GREENSQL_DB_NAME"
      echo "Aborting.."
      exit 0
    fi

    create_user_mysql
    create_tables_mysql
  fi
}

create_tables_mysql()
{
  echo "Creating MySQL tables..."

  MYSQL_SCRIPT="$DOCDIR/greensql-mysql-db.txt"

  if [ -f "${MYSQL_SCRIPT}.gz" ]; then
    if [ -f "${MYSQL_SCRIPT}" ]; then rm -rf ${MYSQL_SCRIPT}; fi

    if [ ! -z $GUNZIP ]; then
      $GUNZIP "${MYSQL_SCRIPT}.gz" > /dev/null 2>&1
    elif [ ! -z $ZCAT ]; then
      $ZCAT "${MYSQL_SCRIPT}.gz" > "${MYSQL_SCRIPT}"
    else
      echo "no gunzip or zcat installed."
      echo "Aborting..."
      exit 0
    fi
  fi

  cat $MYSQL_SCRIPT |  $MYSQL $MRO -f $GREENSQL_DB_NAME > /dev/null 2>&1
}

create_tables_pgsql()
{
  echo "Creating PgSQL tables..."

  PGSQL_SCRIPT="$DOCDIR/greensql-postgresql-db.txt"

  if [ -f "${PGSQL_SCRIPT}.gz" ]; then
    if [ -f "${PGSQL_SCRIPT}" ]; then rm -rf ${PGSQL_SCRIPT}; fi

    if [ ! -z $GUNZIP ]; then
      $GUNZIP "${PGSQL_SCRIPT}.gz" > /dev/null 2>&1
    elif [ ! -z $ZCAT ]; then
      $ZCAT "${PGSQL_SCRIPT}.gz" > "${PGSQL_SCRIPT}"
    else
      echo "no gunzip or zcat installed."
      echo "Aborting..."
      exit 0
    fi
  fi

  if [ "$POSTGRES_LOCATION" = "local" ]; then
    $PSQL -h 127.0.0.1 -f $PGSQL_SCRIPT $GREENSQL_DB_NAME $GREENSQL_DB_USER > /dev/null 2>&1
  elif [ "$POSTGRES_LOCATION" = "remote" ]; then
    $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT -f $PGSQL_SCRIPT $GREENSQL_DB_NAME $GREENSQL_DB_USER > /dev/null 2>&1
  fi
}  

create_user_questions()
{
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
}

create_user_mysql()
{
  echo "Adding MySQL user $GREENSQL_DB_USER..."
  if [ "$MYSQL_HOST" = "localhost" -o "$MYSQL_HOST" = "127.0.0.1" ]
  then
    if ! $MYSQL $MRO $GREENSQL_DB_NAME -f -e "GRANT ALL ON $GREENSQL_DB_NAME.* TO '$GREENSQL_DB_USER'@'localhost' IDENTIFIED BY '${GREENSQL_DB_PWD}'"; then
      echo "Failed to create user $GREENSQL_DB_USER"
      echo "Aborting.."
      exit 0
    fi
  else
   if ! $MYSQL $MRO $GREENSQL_DB_NAME -f -e "GRANT ALL ON $GREENSQL_DB_NAME.* TO '$GREENSQL_DB_USER'@'%' IDENTIFIED BY '${GREENSQL_DB_PWD}'"; then
      echo "Failed to create user $GREENSQL_DB_USER"
      echo "Aborting.."
      exit 0
   fi
  fi
}

create_user_pgsql()
{
  SKIP_USER=0
  EXISTS_USER=0

  if [ "$POSTGRES_LOCATION" = "local" ]; then
    if [ "$POSTGRES_AUTH" = "ident" ]; then
      if su - postgres -c "$PSQL -c \"select * from pg_roles where rolname='$GREENSQL_DB_USER';\" | grep -q \"$GREENSQL_DB_USER\""; then
        SKIP_USER=0
        EXISTS_USER=1
        echo " "
        echo "user $GREENSQL_DB_USER already exists"
        echo -n "Do you want to drop and recreate user $GREENSQL_DB_USER [Y/n]: "
        read cont
        if [ "$cont" = "N" ] || [ "$cont" = "n" ]; then
          SKIP_USER=1
        else
          SKIP_USER=0
        fi
      fi

      if [ "$SKIP_USER" = "0" ]; then
        if [ "$EXISTS_USER" = "1" ]; then
          echo "Dropping User $GREENSQL_DB_USER"
          if ! su - postgres -c "$PSQL -c \"DROP USER $GREENSQL_DB_USER\""; then
            echo "Failed to drop user $GREENSQL_DB_USER"
            echo "Aborting.."
            exit 0
          fi
        fi

        echo "Adding User $GREENSQL_DB_USER"
        if ! su - postgres -c "$PSQL -c \"CREATE USER $GREENSQL_DB_USER WITH PASSWORD '$GREENSQL_DB_PWD';\""; then
          echo "Failed to create user $GREENSQL_DB_USER"
          echo "Aborting.."
          exit 0
        fi
      fi
    elif [ "$POSTGRES_AUTH" = "md5" ]; then
      if $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "select * from pg_roles where rolname='$GREENSQL_DB_USER';" | grep -q "$GREENSQL_DB_USER"; then
        SKIP_USER=0
        EXISTS_USER=1
        echo " "
        echo "user $GREENSQL_DB_USER already exists"
        echo -n "Do you want to drop and recreate user $GREENSQL_DB_USER [Y/n]: "
        read cont
        if [ "$cont" = "N" ] || [ "$cont" = "n" ]; then
          SKIP_USER=1
        else
          SKIP_USER=0
        fi
      fi

      if [ "$SKIP_USER" = "0" ]; then
        if [ "$EXISTS_USER" = "1" ]; then
          echo "Dropping User $GREENSQL_DB_USER" 
          if ! $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "DROP USER $GREENSQL_DB_USER;"; then
            echo "Failed to drop user $GREENSQL_DB_USER"
            echo "Aborting.."
            exit 0
          fi
        fi

        echo "Adding User $GREENSQL_DB_USER"
        if ! $PSQL -h 127.0.0.1 postgres $POSTGRES_ADMIN_USER -c "CREATE USER $GREENSQL_DB_USER WITH PASSWORD '$GREENSQL_DB_PWD';"; then
          echo "Failed to create user $GREENSQL_DB_USER"
          echo "Aborting.."
          exit 0
        fi
      fi
    fi
  elif [ "$POSTGRES_LOCATION" = "remote" ]; then
    if $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "select * from pg_roles where rolname='$GREENSQL_DB_USER';" | grep -q "$GREENSQL_DB_USER"; then
      SKIP_USER=0
      EXISTS_USER=1
      echo " "
      echo "user $GREENSQL_DB_USER already exists"
      echo -n "Do you want to drop and recreate user $GREENSQL_DB_USER [Y/n]: "
      read cont
      if [ "$cont" = "N" ] || [ "$cont" = "n" ]; then
        SKIP_USER=1
      else
        SKIP_USER=0
      fi
    fi

    if [ "$SKIP_USER" = "0" ]; then
      if [ "$EXISTS_USER" = "1" ]; then
        echo "Dropping User $GREENSQL_DB_USER"
        if ! $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "DROP USER $GREENSQL_DB_USER;"; then
          echo "Failed to drop user $GREENSQL_DB_USER"
          echo "Aborting.."
          exit 0
        fi

        echo "Adding User $GREENSQL_DB_USER"
        if ! $PSQL -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_ADMIN_USER postgres -c "CREATE USER $GREENSQL_DB_USER WITH PASSWORD '$GREENSQL_DB_PWD';"; then
          echo "Failed to create user $GREENSQL_DB_USER"
          echo "Aborting.."
          exit 0
        fi
      fi
    fi
  fi
}

update_greensql_config()
{
  echo "Modifing $CONF..."

  if [ $SQL = "pgsql" ]; then
    SQL="postgresql"
  fi

  # save start and end of the config file
  start_cfg=`perl -p0777 -e 's/\[database\].*$//s' $CONF`
  end_cfg=`perl -p0777 -e 's/^.*\[database\][^\[]*\[/\[/s' $CONF`

  echo "$start_cfg

[database]" > $CONF

  if [ "$SQL" = "postgresql" ]; then
    if [ -z $POSTGRES_HOST ]; then
      POSTGRES_HOST="127.0.0.1"
    fi

    echo "dbhost=$POSTGRES_HOST" >> $CONF
  elif [ "$SQL" = "mysql" ]; then
    if [ -z $MYSQL_HOST ]; then
      MYSQL_HOST="127.0.0.1"
    fi

    echo "dbhost=$MYSQL_HOST" >> $CONF
  fi

  if [ $SQL = "mysql" ]; then
    DBTYPE=mysql
  elif [ $SQL = "postgresql" ]; then
    DBTYPE=pgsql
  else
    DBTYPE=mysql
  fi

  echo "dbname=$GREENSQL_DB_NAME
dbuser=$GREENSQL_DB_USER
dbpass=$GREENSQL_DB_PWD
dbtype=$DBTYPE" >> $CONF

  if [ "$SQL" = "postgresql" ] && [ "$POSTGRES_LOCATION" = "remote" ]; then
    if [ "$POSTGRES_PORT" != "" ] && [ "$POSTGRES_PORT" != "5432" ]; then
      echo "dbport=${POSTGRES_PORT}" >> $CONF
    else
      echo "# dbport=5432" >> $CONF
    fi
  elif [ "$SQL" = "mysql" ]; then
    if [ "$MYSQL_PORT" != "" ] && [ "$MYSQL_PORT" != "3306" ]; then
      echo "dbport=${MYSQL_PORT}" >> $CONF
    else
      echo "# dbport=3306" >> $CONF
    fi
  fi

  echo "
$end_cfg" >> $CONF
}

# execution of the script start here:
main_fn
update_greensql_config


########################
##   WEB CONFIG FILE  ##
########################

if test -f $CONFWEB
then
  cp -a $CONFWEB ${CONFWEB}-old
else
  if [ -f $DOCDIR/config.php ]; then
    cp $DOCDIR/config.php $CONFWEB
  fi

  cp -a $CONFWEB ${CONFWEB}-old
fi

## code to merge the new variables now in the configuration file also to the web console conf file
perl <<'EOF'

my @data;
my ($db_type,$db_name,$db_host,$db_port,$db_user,$db_pass);
my $CONFWEB;

$CONF = '/etc/greensql/greensql.conf';
$CONFWEB = '/usr/share/greensql-fw/config.php';

##########################################
## extract the data from the config file #
##########################################
open(CONF,"$CONF") or die $!;
while(<CONF>) { $data .= $_; }
close(CONF);

## quotes before split
$data =~ s/\\/\\\\/g;
@data = split("\n", $data);
foreach my $line ( @data )
{
  $line =~ s/\#.*//;

  ## db type (commercial version)
  if ($line =~ m/dbtype\s*=\s*(.*)/i) {
    $db_type = $1;
  }

  ## db host
  if ($line =~ m/dbhost\s*=\s*(.*)/i) {
    $db_host = $1;
  }

  ## db port
  if ($line =~ m/dbport\s*=\s*(\d*)/i) {
    $db_port = $1;
  }

  # db name
  if ($line =~ m/dbname\s*=\s*(.*)/i) {
    $db_name = $1;
  }

  ## db user
  if ($line =~ m/dbuser\s*=\s*(.*)/i) {
    $db_user = $1;
  }

  # db pass
  if ($line =~ m/dbpass\s*=\s*(.*)/i) {
    $db_pass = $1;
  }
}

$data = '';
open(CONFWEB,$CONFWEB) or die $!;
while(<CONFWEB>) {
  $data .= $_;
}
close(CONFWEB);

@data = split("\n", $data);
open(CONFWEB,">$CONFWEB") or die $!;
foreach my $data ( @data )
{
  if ( $data =~ m/\#.*/) {
    ## happens alot so lets speed up process
    print CONFWEB $data . "\n";
  }
  elsif ($db_type && $data =~ m/db_type/) {
    print CONFWEB '$db_type = "' . $db_type . '";' . "\n";
  }
  elsif ($db_host && $data =~ m/db_host/) {
    print CONFWEB '$db_host = "' . $db_host . '";' . "\n";
  }
  elsif ($db_port && $data =~ m/db_port/) {
    print CONFWEB '$db_port = "' . $db_port . '";' . "\n";
  }
  elsif ($db_name && $data =~ m/db_name/) {
    print CONFWEB '$db_name = "' . $db_name . '";' . "\n";
  }
  elsif ($db_user && $data =~ m/db_user/) {
    print CONFWEB '$db_user = "' . $db_user . '";' . "\n";
  }
  elsif ($db_pass && $data =~ m/db_pass/) {
    print CONFWEB '$db_pass = "' . $db_pass . '";' . "\n";
  }
  else { print CONFWEB $data . "\n";}
}
close(CONFWEB);
EOF

