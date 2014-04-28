#!/bin/sh
/usr/local/bin/conky -c conky-lcd.rc -i 1 | /usr/bin/awk 'BEGIN {print "c"};{ printf "p:%20s\n", $0 }' | nc 127.0.0.1 9999
sleep 10
