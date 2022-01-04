#!/bin/bash

DIR=projs
NAME="arch_dep"
NODE="node30"

DM=$DIR/$NAME

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

ssh $NODE "mkdir $DIR ; mkdir $DM "

rsync -avz . $NODE:$DM
