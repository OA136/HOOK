#!/bin/sh

find . -type d > tmp_include_config
find . -iname "*.c" > config_src_c
awk '{print "CFLAGS += -I" $1} ' temp_include_config > cflags_include
awk '{gsub(/\.c/,".o"); print} ' config_src_c > obj_config
awk '{print "marvell-y += " $1} ' obj_config > rule
