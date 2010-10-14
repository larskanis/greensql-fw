
drop table if exists query;

CREATE table query
(
queryid	       SERIAL primary key,
proxyid        int NOT NULL default '0',
perm           smallint NOT NULL default 1,
db_name        varchar(50) NOT NULL,
query          text NOT NULL
);

CREATE INDEX query_idx on query(proxyid,db_name);

drop table if exists proxy;

CREATE table proxy
(
proxyid        SERIAL primary key,
proxyname      varchar(50) NOT NULL default '',
frontend_ip    varchar(20) NOT NULL default '',
frontend_port  smallint NOT NULL default 0,
backend_server varchar(50) NOT NULL default '',
backend_ip     varchar(20) NOT NULL default '',
backend_port   smallint NOT NULL default 0,
dbtype         varchar(20) NOT NULL default 'mysql',
status         smallint NOT NULL default '1'
);
insert into proxy values (1,'Default MySQL Proxy','127.0.0.1',3305,'localhost','127.0.0.1',3306,'mysql',1);
insert into proxy values (2,'Default PgSQL Proxy','127.0.0.1',5431,'localhost','127.0.0.1',5432,'pgsql',1);
select setval('proxy_proxyid_seq'::regclass,2);

drop table if exists db_perm;

CREATE table db_perm
(
dbpid          SERIAL primary key,
proxyid        int NOT NULL default '0',
db_name        varchar(50) NOT NULL,
perms          bigint NOT NULL default '0',
perms2         bigint NOT NULL default '0',
status         smallint  NOT NULL default '0',
sysdbtype      varchar(20) NOT NULL default 'user_db',
status_changed TIMESTAMP
);

CREATE INDEX db_perm_idx on db_perm(proxyid, db_name);

insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (1,0,'default mysql db', 'default_mysql');
insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (2,0,'no-name mysql db', 'empty_mysql');
insert into db_perm (dbpid, proxyid, db_name, sysdbtype) values (3,0,'default pgsql db', 'default_pgsql');
select setval('db_perm_dbpid_seq'::regclass,3);

drop table if exists admin;

CREATE table admin
(
adminid        SERIAL PRIMARY KEY,
name           varchar(50) NOT NULL default '',
pwd            varchar(50) NOT NULL default '',
email          varchar(50) NOT NULL default ''
);

insert into admin values(1,'admin','37fa265330ad83eaa879efb1e2db6380896cf639','');
select setval('admin_adminid_seq'::regclass,1);

drop table if exists alert;
CREATE table alert
(
alertid             SERIAL PRIMARY KEY,
agroupid            int default '0',
event_time          timestamp,
risk                smallint NOT NULL default '0',
block               smallint NOT NULL default '0',
dbuser              varchar(50) NOT NULL default '',
userip              varchar(50) NOT NULL default '',
query               text NOT NULL,
reason              text NOT NULL
);

CREATE INDEX alert_agroupid_idx on alert(agroupid);

drop table if exists alert_group;
CREATE table alert_group
(
agroupid            SERIAL PRIMARY KEY,
proxyid             int NOT NULL default '1',
db_name             varchar(50) NOT NULL default '',
update_time         timestamp,
status              smallint NOT NULL default 0,
pattern             text NOT NULL
);

create index alert_group_idx on alert_group(update_time);
