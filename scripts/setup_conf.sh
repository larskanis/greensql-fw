#!/bin/sh

if [ "$(id -u)" != "0" ]; then
    echo
    echo "Please run this script as root"
    echo
    exit 1
fi

mkdir -p /etc/greensql
cp ../conf/* /etc/greensql/ -r
chown greensql:greensql /etc/greensql -R
chmod 700 /etc/greensql
echo "done..."
