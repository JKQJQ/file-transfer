#!/bin/bash

pkill -f file_server.sh

rm -rf output/
mkdir output/

bash ./build-server.sh

# don't forget the '/' in the end!, no space around =
DATA_DIR=/data/team-10/large/order1_compress/

# executable file
EXE=server/build/file_server

nohup ${EXE} 40017 ${DATA_DIR} > output/server1.out &
nohup ${EXE} 40018 ${DATA_DIR} > output/server2.out &
nohup ${EXE} 40019 ${DATA_DIR} > output/server3.out &
