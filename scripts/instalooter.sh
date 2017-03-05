#!/usr/bin/env bash

# first arg is tag

while true ; do

instaLooter hashtag $1 $1/download/ -n 4000 --new -T {id}.{ownerid}.{datetime}

sleep $2;

done

