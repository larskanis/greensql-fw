#!/bin/sh
#

# PROVIDE: greensql-fw
# REQUIRE: DAEMON mysql 
# KEYWORD: shutdown

#
# Add the following line to /etc/rc.conf to enable greensql:
# greensql_enable (bool):	Set to "NO" by default.
#			Set it to "YES" to enable greensql.
#
GREENSQL_LOG=/var/log/greensql.log
touch $GREENSQL_LOG
chmod 0644 $GREENSQL_LOG
chown greensql:greensql $GREENSQL_LOG

greensql_user=${greensql_user:-"greensql"}
greensql_group=${greensql_group:-"greensql"}

. /etc/rc.subr

name="greensql"
rcvar=`set_rcvar`

load_rc_config $name

: ${greensql-fw_enable="NO"}

command="/usr/local/sbin/greensql-fw"
command_args="-p /usr/local/etc/greensql/ >/dev/null 2>&1 &"
pidfile="/var/run/greensql.pid"
required_files="/usr/local/etc/greensql/greensql.conf"
stop_cmd="greensql_stop"

greensql_stop()
{
killall greensql-fw
}

run_rc_command "$1"


