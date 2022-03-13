#!/bin/bash

rm -rf output/
mkdir output/

nohup server/build/file_server 40017 /data/team-10/remote/ > output/server1.out &
nohup server/build/file_server 40018 /data/team-10/remote/ > output/server2.out &
nohup server/build/file_server 40019 /data/team-10/remote/ > output/server3.out &