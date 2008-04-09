#!/bin/sh

if [ "$(id -u)" != "0" ]; then
    echo
    echo "Please run this script as root"
    echo
    exit 1
fi

touch /var/log/greensql.log
chown greensql:greensql /var/log/greensql.log
chmod 666 /var/log/greensql.log
cp greensql.rotate /etc/logrotate.d/greensql
echo "done..."
