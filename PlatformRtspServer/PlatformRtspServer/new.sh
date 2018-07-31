#!/bin/bash


HOSTNAME="127.0.0.1"
PORT="3306"
USERNAME="root"
PASSWD="goscam66%%DB" #数据库密码

DBNAME="rtsp_db" #要创建的数据库名称
TABLENAME="device_rec" #要创建的表

#MYSQL_CMD="mysql -h${HOSTNAME} -P${PORT} -u${USERNAME} -p${PASSWD}"
MYSQL_CMD="mysql -P${PORT} -u${USERNAME} -p${PASSWD}"

CREATE_DB_SQL_CMD="create database IF NOT EXISTS ${DBNAME}"

${MYSQL_CMD} -e "${CREATE_DB_SQL_CMD}"

if [ $? -ne 0 ]
then
	echo "create databases ${DBNAME} failed ..."
	exit 1
fi

CREATE_TABLE_SQL_CMD="create table ${TABLENAME}(
id BIGINT NOT NULL AUTO_INCREMENT,
guid VARCHAR(64) NOT NULL,
times VARCHAR(64) NOT NULL,
starttime int unsigned not null,
rtsp int unsigned,
hls int unsigned,
PRIMARY KEY(id),
UNIQUE KEY(guid));"

${MYSQL_CMD} ${DBNAME} -e "${CREATE_TABLE_SQL_CMD}"

if [ $? -ne 0 ]
then
 echo "create table ${DBNAME}.${TABLENAME} failed ..."
fi

#alter table device_rec add column rtsp int unsigned;
#alter table device_rec add column hls int unsigned;
