#!/bin/sh
#called by /etc/crontab all 15min
logger "Getting data from sdm120 and save to db"
tmpfile=/tmpram/sdm120.tmp.sql

if [ -f $tmpfile ]; then
	rm $tmpfile
fi
#flush input:
tail /dev/modbus_dongle>/dev/null
sdm120.py -sql /dev/modbus_dongle > $tmpfile

if [ -f $tmpfile ]; then
	sed -i '1iINSERT INTO sdm120\n' $tmpfile
	mysql -D sonnenkind < $tmpfile
	[ "$?" != "0" ] && logger "$0 - mysql access failed" || :
else
	logger "$0 - outfile wurde nicht erstell!"
	exit 1
fi

