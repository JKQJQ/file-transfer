#!/bin/bash

######## VARIABLES ######################

SERVER_IP=10.216.68.189
FORWARD_DIR=/data/team-10/forward/

####### END ############################

pkill -f file_client
pkill -f file_server

rm -rf output/
mkdir -p output/

bash ./build-server.sh
bash ./build-client.sh

CLIENT_EXE=client/build/file_client
SERVER_EXE=server/build/file_server
N_WORKER=3
SUFFIX=.zst

mkdir -p ${FORWARD_DIR}

# download as client
nohup ${CLIENT_EXE} ${N_WORKER} 0 12350 ${SERVER_IP} 12353 ${FORWARD_DIR} ${SUFFIX} > output/fclient1.out &
nohup ${CLIENT_EXE} ${N_WORKER} 1 12351 ${SERVER_IP} 12354 ${FORWARD_DIR} ${SUFFIX} > output/fclient2.out &
nohup ${CLIENT_EXE} ${N_WORKER} 2 12352 ${SERVER_IP} 12355 ${FORWARD_DIR} ${SUFFIX} > output/fclient3.out &

# upload as server
nohup ${SERVER_EXE} 12350 ${FORWARD_DIR} > output/fserver1.out &
nohup ${SERVER_EXE} 12351 ${FORWARD_DIR} > output/fserver2.out &
nohup ${SERVER_EXE} 12352 ${FORWARD_DIR} > output/fserver3.out &