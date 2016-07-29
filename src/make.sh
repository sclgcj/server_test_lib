#!/bin/bash

#This file is used to set some global variables, 
#and  start the make command

export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/:$PKG_CONFIG_PATH


echo $1

make $1
