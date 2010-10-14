--
-- This file contains GreenSQL configuration database structure.
--
-- Step 1 - Configuration database
-- You can create configuration database using the following
-- MySQL command:
-- 
-- mysql> create database greendb;
--
-- Step 2 - Create MySQL user
-- Second, create green user. It will be used to access configuration db.
--
-- mysql> GRANT ALL PRIVILEGES ON greendb.* TO 'green'@'%' IDENTIFIED BY 'pwd';
--
-- IMPORTANT: change "pwd" to other password. Don't forget to update
-- /etc/greensql/greensql.conf file with new password value.
--
-- Step 3 - Flush privileges
-- Run the following mysql command:
--
-- mysql> flush privileges;
--
-- Step 4- Create all tables
-- Run the following shell command:
--
-- cat greensql-mysql-db.txt | mysql greendb
--

drop table if exists query;

CREATE table query
(
queryid int unsigned NOT NULL auto_increment primary key,
proxyid        int unsigned NOT NULL default '0',
perm           smallint unsigned NOT NULL default 1,
db_name        char(50) NOT NULL,
query          text NOT NULL,
INDEX(proxyid,db_name)
) DEFAULT CHARSET=utf8;

drop table if exists proxy;

CREATE table proxy
(
proxyid        int unsigned NOT NULL auto_increment primary key,
proxyname      char(50) NOT NULL default '',
frontend_ip    char(20) NOT NULL default '',
frontend_port  smallint unsigned NOT NULL default 0,
backend_server char(50) NOT NULL default '',
backend_ip     char(20) NOT NULL default '',
backend_port   smallint unsigned NOT NULL default 0,
dbtype         char(20) NOT NULL default 'mysql',
status         smallint unsigned NOT NULL default '1'
) DEFAULT CHARSET=utf8;

insert into proxy values (1,'Default MySQL Proxy','127.0.0.1',3305,'localhost','127.0.0.1',3306,'mysql',1);
insert into proxy values (2,'Default PgSQL Proxy','127.0.0.1',5431,'localhost','127.0.0.1',5432,'pgsql',1);

drop table if exists db_perm;

CREATE table db_perm
(
dbpid          int unsigned NOT NULL auto_increment primary key,
proxyid        int unsigned NOT NULL default '0',
db_name        char(50) NOT NULL,
perms          bigint unsigned NOT NULL default '0',
perms2         bigint unsigned NOT NULL default '0',
status         smallint unsigned NOT NULL default '0',
sysdbtype      char(20) NOT NULL default 'user_db',
status_changed datetime NOT NULL default '00-00-0000 00:00:00',
INDEX (proxyid, db_name)
) DEFAULT CHARSET=utf8;

insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (1,0,'default mysql db', 'default_mysql');
insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (2,0,'no-name mysql db', 'empty_mysql');
insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (3,0,'default pgsql db', 'default_pgsql');

drop table if exists admin;

CREATE table admin
(
adminid         int unsigned NOT NULL auto_increment primary key,
name           char(50) NOT NULL default '',
pwd            char(50) NOT NULL default '',
email          char(50) NOT NULL default ''
) DEFAULT CHARSET=utf8;

insert into admin values(1,'admin',sha1('pwd'),'');

drop table if exists alert;

CREATE table alert
(
alertid             int unsigned NOT NULL auto_increment primary key,
agroupid            int unsigned NOT NULL default '0',
event_time          datetime NOT NULL default '00-00-0000 00:00:00',
risk                smallint unsigned NOT NULL default '0',
block               smallint unsigned NOT NULL default '0',
dbuser              varchar(50) NOT NULL default '',
userip              varchar(50) NOT NULL default '',
query               text NOT NULL,
reason              text NOT NULL,
INDEX (agroupid)
) DEFAULT CHARSET=utf8;

drop table if exists alert_group;

CREATE table alert_group
(
agroupid            int unsigned NOT NULL auto_increment primary key,
proxyid             int unsigned NOT NULL default '1',
db_name             char(50) NOT NULL default '',
update_time         datetime NOT NULL default '00-00-0000 00:00:00',
status              smallint NOT NULL default 0,
pattern             text NOT NULL,
INDEX(update_time)
) DEFAULT CHARSET=utf8;


