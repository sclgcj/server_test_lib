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
proj=""
cmd_arg=""
opt_file=""
LD_LIBRARY_PATH=`pwd`/lib/comm_lib:$LD_LIBRARY_PATH

export LD_LIBRARY_PATH

echo $LD_LIBRARY_PATH

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
			proj="$OPTARG"
			path="./config/$OPTARG"
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
		"udt_mobile")
			rm -rf mobile_log.txt
			rm -rf mobile_rt_log
			exe_suffix="udt_mobile_client"
			;;
		"udt_gateway")
			rm -rf gateway_log.txt
			rm -rf gateway_rt_log
			exe_suffix="udt_gateway_client"
			;;
	esac
fi

opt_file="config/$proj/opt_cmd.txt"
if [ -f $opt_file ]
then
	cmd_arg=`cat $opt_file | tr '\n' ' '`
else
	echo "请输入命令行参数, 一个参数为一行, 以end结束:"
	while [ ""$line != "end" ]
	do
		read line	
		echo "line = $line"
		if [ ""$line == "end" ]
		then
			break
		fi
		cmd_arg="$cmd_arg $line"
		echo "cmd_arg = $cmd_arg"
	done
fi

#设置配置文件,删除文件以及可执行文件路径
conf_file=$path"/"$nat_conf
other_file=$exe_suffix"_*"
exe_file=$exe_prefix"/"$exe_suffix

echo `pwd` > ./config/comm/curr_dir.txt
echo $proj >> ./config/comm/curr_dir.txt
echo "udt_gateway_client config/$proj/gateway.toml" > config/$proj/nat_conf.txt
echo "udt_mobile_client config/$proj/mobile.toml" >> config/$proj/nat_conf.txt

cur_time=`date +%Y_%m_%d_%H_%M_%S`
echo cur_time
tar -cJf ./log/log_$proj.$cur_time.tar.xz ./log/$proj
rm -rf ./log/$proj
mkdir -p ./log/$proj
#打印配置结果
echo $path
echo "other_file = "$other_file
echo "path = "$path
echo "conf_file="$conf_file
echo "exe_file="$exe_file
echo "dev_type = "$dev_type
echo "total = "$total
echo "sec = "$sec
echo "cmd_arg = "$cmd_arg

rm -rf $other_file

for i in $(seq $total)
do 
	killall -9 $exe_file
	ps -A -o stat,ppid,pid,cmd | grep -e '^[Zz]' | awk '{print $2}' | xargs kill -9
	echo "$exe_file $sdaemon -f $conf_file $cmd_arg"
	$exe_file $sdaemon -f $conf_file $cmd_arg
	for j in $(seq $sec)
	do
		echo $j
		sleep 1
	done
	#sleep $sec
done
		
result_file=""$proj_$exe_file"_*_result.txt"

if [ -e $result_file ]
then
	echo $result_file
	cp $result_file .
fi


