#!/bin/sh

if [ "$(id -u)" != "0" ]; then
    echo
    echo "Please run this script as root"
    echo
    exit 1
fi

mysqladmin create greendb
cat ../db/mysql_create_tables.sql | mysql greendb
echo "GRANT ALL PRIVILEGES ON greendb.* TO 'green'@'%' IDENTIFIED BY 'pwd';" | mysql
mysqladmin flush-privileges

echo "done..."
