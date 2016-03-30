#!/bin/bash

nat_conf="nat_conf.txt"
#path="./config/udt_nat_server/"
path=""
exe_prefix=""
exe_suffix=""
prefix=""
exe_file=""
conf_file=""
dev_type=""
other_file=""
sdaemon=""
sec=0
total=1

#杀掉上次遗留的僵尸进程
ps -A -o stat,ppid,pid,cmd | grep -e '^[Zz]' | awk '{print $2}' | xargs kill -9

#获取选项配置
while getopts "dt:n:s:f:e:p:" arg
do
	case $arg in
		't')
			dev_type=$OPTARG
			;;
		'n')
			total=`echo $OPTARG | sed 's/^0*//'`
			;;
		's')
			sec=`echo $OPTARG | sed 's/^0*//'`
			;;
		'f')
			path=$OPTARG
			;;
		'e')
			exe_suffix=$OPTARG
			;;
		'p')
			path="./conf/$OPTARG"
			exe_prefix="./bin/$OPTARG"
			;;
		'd')
			sdaemon="-d"
			;;
	esac
done

#获取默认可执行文件名
if [ "X$exe_suffix" == "X" ]
then
	echo "no exe set"
	case $dev_type in
		"mobile")
			exe_suffix="mobile"
			nat_conf="mobile_conf.txt"
			;;
		"gateway")
			exe_suffix="gateway"
			nat_conf="gateway_conf.txt"
			;;
	esac
fi

#设置配置文件,删除文件以及可执行文件路径
conf_file=$path"/"$nat_conf
other_file=$exe_suffix"_*"
exe_file=$exe_prefix"/"$exe_suffix

#打印配置结果
echo $path
echo "other_file = "$other_file
echo "path = "$path
echo "conf_file="$conf_file
echo "exe_file="$exe_file
echo "dev_type = "$dev_type
echo "total = "$total
echo "sec = "$sec

rm -rf $other_file

for i in $(seq $total)
do 
	killall -9 $exe_file
	ps -A -o stat,ppid,pid,cmd | grep -e '^[Zz]' | awk '{print $2}' | xargs kill -9
	$exe_file $sdaemon -f $conf_file
	for j in $(seq $sec)
	do
		echo $j
		sleep 1
	done
	#sleep $sec
done
		
#result_file=""$exe_file"_*_result.txt"

#echo $result_file

#cp $result_file .

