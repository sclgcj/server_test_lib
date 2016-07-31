#!/bin/bash

user='mark:x:0:0:this is a test user:/var/mark:nologin'
i=1

ch=`echo $1 | grep ','`
if [ "$ch" == "" ]
then
	exit
fi
while ((1==1))
do
	split=`echo $1|cut -d "," -f$i`
	if [ "$split" != "" ]
	then
		((i++))
		echo $split
	else 
		break
	fi
done
