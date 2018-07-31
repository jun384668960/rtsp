#!/bin/bash

args=$#
uid=$1
uidlen=${#uid}
if [ ${args} -lt 0 ]; then
        echo "parameter error"
        exit 0
fi

if [ ${uidlen} -eq 15 ] || [ ${uidlen} -eq 28 ]; then
        result="$(mysql -uroot -pUlife@2018 -h 127.0.0.1 rtsp_db -e "select * from device_rec where guid='${uid}';")"
            echo "${result}"
else
        echo "uid length error ${uidlen}"
fi

exit 0
