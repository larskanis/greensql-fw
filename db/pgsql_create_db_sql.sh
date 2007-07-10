#!/bin/sh

su postgres
createuser --no-adduser --no-createdb --encrypted  --pwprompt green
createdb greendb -O greenuser

