
drop table if exists query;

CREATE table query
(
queryid int unsigned NOT NULL auto_increment primary key,
proxyid        int unsigned NOT NULL default '0',
perm           smallint unsigned NOT NULL default 1,
db_name        char(50) NOT NULL,
query          text NOT NULL,
INDEX(proxyid,db_name)
);

drop table if exists proxy;

CREATE table proxy
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
);
insert into proxy values (0,'Default Proxy',INET_ATON('127.0.0.1'),3305,
'localhost',INET_ATON('127.0.0.1'),3306,'mysql',1);


drop table if exists db_perm;

CREATE table db_perm
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
);

insert into db_perm (dbpid, proxyid, db_name) values (0,0,'defaultdb');


drop table if exists user;

CREATE table user
(
userid         int unsigned NOT NULL auto_increment primary key,
name           char(50) NOT NULL default '',
pwd            char(50) NOT NULL default '',
email          char(50) NOT NULL default ''
);

insert into user values(0,'admin',sha1('pwd'),'');

drop table if exists alert;

CREATE table alert
(
alertid             int unsigned NOT NULL auto_increment primary key,
agroupid            int unsigned NOT NULL default '0',
event_time          datetime NOT NULL,
risk                smallint unsigned NOT NULL default '0',
block               smallint unsigned NOT NULL default '0',
user                varchar(50) NULL default '',
query               text NOT NULL,
reason              text NOT NULL
);

drop table if exists alert_group;

CREATE table alert_group
(
agroupid            int unsigned NOT NULL auto_increment primary key,
proxyid             int unsigned NOT NULL default '1',
db_name             char(50) NOT NULL default '',
update_time         datetime NOT NULL,
status              smallint NOT NULL default 0,
pattern             text NOT NULL,
INDEX(update_time)
);


