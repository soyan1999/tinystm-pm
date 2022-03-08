#!/bin/bash

begin_dir=$(pwd)
clean_command="make clean"
make_command="make all"

if [ "$1" = "pstm" ]; then
  lib_dir="persist"
else
  lib_dir="pmdk"
fi

if [ $# -eq 2 ] && [ "$2" = "debug" ]; then
  make_command="$make_command CFG=debug"
fi

echo "$make_command"

cd ../../$lib_dir

eval $clean_command
eval $make_command

cd ..

eval $clean_command
eval $make_command

cd $begin_dir