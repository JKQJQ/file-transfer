#!/bin/bash

pkill -f file_server.sh

rm -rf output/
mkdir output/

bash ./build-server.sh

nohup server/build/file_server 40017 /data/team-10/remote/ > output/server1.out &
nohup server/build/file_server 40018 /data/team-10/remote/ > output/server2.out &
nohup server/build/file_server 40019 /data/team-10/remote/ > output/server3.out &
# nohup server/build/file_server 40020 /data/team-10/remote/ > output/server4.out &
# nohup server/build/file_server 40021 /data/team-10/remote/ > output/server5.out &
