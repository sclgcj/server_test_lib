#!/bin/bash

new_file=""
proj="" num=""


if [ ""$1 != "" ]
then
	proj=$1
else
	if [ -e ./last_proj.txt ]
	then
		proj=`tail -n 1 ./last_proj.txt | tr '\n' '\0'`
	else 
		if [ -e ./proj_conf ]
		then
			proj=`tail -n 1 ./proj_conf | tr '\n' '\0'`
		else
			echo "no lib_conf file"
			exit
		fi
	fi
fi

echo "proj = $proj"

lib=${proj:6}
pre=${proj:0:6}
echo "pre = $pre"
#`echo $proj | grep lib | tr '\n' '\0' | tr '\./lib/' '\0'`
echo "lib = $lib"

if [ -e ./last_proj.txt ]
then
	sed -i "/\<$lib\>/d" last_proj.txt
	echo $proj >> ./last_proj.txt
else
	echo $proj > ./last_proj.txt
fi

if [ ""$pre != "./lib/" ]
then
	files=`ls -lt ./$proj/$proj/*.[ch] | awk '{print $9}' | tr '\n' ' '`
else
	files=`ls -lt ./lib/$lib/$lib/*.[ch] | awk '{print $9}' | tr '\n' ' '`
fi
echo "files = $files"

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

echo "newfile = $new_file"


vim $new_file

echo hhhh
