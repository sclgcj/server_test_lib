#!/bin/bash

new_file=""
proj=""
num=""

if [ ""$1 != "" ]
then
	proj=$1
else
	if [ -e ./last_proj.txt ]
	then
		proj=`cat ./last_proj.txt | tr '\n' '\0'`
	else
		echo "erer"
		if [ ! -e "./proj_conf" ]
		then
			echo "没有创建新项目，请使用make new proj=项目创建新项目"
			exit
		fi
		proj_dir=`cat proj_conf | tr '\n' ' '`
		flag=0
		echo "flsg = "$flag
		echo "proj_dir = $proj_dir"
		while [ $flag -eq 0 ]
		do
			echo "请选择想要处理的工程序号:"
			for i in $proj_dir
			do
				echo "1 $i"
			done
			read num
			cmt=1
			for i in $proj_dir
			do
				if [ ""$cmt == $num ]
				then
					echo "erer"
					flag=1
					proj=$i
					break;
				fi
				let "cmt+=1"
			done
			echo "flag = $flag"
			if [ $flag -eq 1 ]
			then
				break
			fi
			echo "错误选项，请选择正确的序号!"
			echo ""
			echo ""
			continue
		done
	fi

fi

echo $proj > ./last_proj.txt
files=`ls -lt ./$proj/$proj/ | awk '{print $9}'`
echo "files = $files"

echo $files


find `pwd` -name "*.[ch]" -o -name "*.cpp" > cscope.files  
cscope -bR -i cscope.files  
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/:$PKG_CONFIG_PATH
#cscpe -Rbq

for i in $files;
do
	echo $i
	if [ -d $i ]
	then
		continue
	fi
	new_file=$i
	break
done

echo "newfile = $proj/$proj/$new_file"


vim $proj/$proj/$new_file

echo hhhh
