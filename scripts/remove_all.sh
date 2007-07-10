#!/bin/sh

# the following script will remove all part of the greensql files
# configuration files will be left

if [ "$(id -u)" != "0" ]; then
    echo
    echo "Please run this script as root"
    echo
    exit 1
fi

#remove application binatries
rm -rf /usr/sbin/greensql-fw
rm -rf /sbin/greensql-fw
rm -rf /etc/init.d/greensql
rm -rf /var/log/greensql.log
rm -rf /etc/logrotate.d/greensql
#removing user and his group
userdel greensql >/dev/null 2>&1
groupdel greensql >/dev/null 2>&1

echo "You must remove greensql configuration files manually in /etc/greensql"

