#!/bin/sh

if [ "$(id -u)" != "0" ]; then
    echo
    echo "Please run this script as root"
    echo
    exit 1
fi

groupadd greensql >/dev/null 2>&1
useradd -M -g greensql -s /dev/null greensql >/dev/null 2>&1
echo "done..."
