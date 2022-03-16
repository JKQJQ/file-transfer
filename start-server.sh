#!/bin/bash

######## VARIABLES ######################

DATA_DIR=/data/team-10/test/order1/

####### END ############################

pkill -f file_server

rm -rf output/server*
mkdir -p output/

bash ./build-server.sh

# executable file
EXE=server/build/file_server

nohup ${EXE} 12350 ${DATA_DIR} > output/server1.out &
nohup ${EXE} 12351 ${DATA_DIR} > output/server2.out &
nohup ${EXE} 12352 ${DATA_DIR} > output/server3.out &
