rgs=$#
uid=$1
uidlen=${#uid}
if [ ${args} -lt 0 ]; then
        echo "parameter error"
exit 0
fi

if [ ${uidlen} -eq 15 ] || [ ${uidlen} -eq 28 ]; then
    result="$(mysql -h 127.0.0.1 -uroot -pUlife@2018 rtsp_db -e "update device_rec set times=0,rtsp=0,hls=0 where GUID='${uid}';")"
    echo ${result}
else
    echo "uid length error ${uidlen}"
fi

exit 0
