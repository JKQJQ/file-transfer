#!/bin/bash

pkill -f file_client
pkill -f file_server
rm -rf output/
mkdir output/

bash ./build-server.sh
bash ./build-client.sh

/data/team-10/forward/

CLIENT_EXE=client/build/file_client
SERVER_EXE=server/build/file_server
N_WORKER=3
SERVER_IP=10.216.68.190
FORWARD_DIR=/data/team-10/forward/
SUFFIX=.zst

# download as client
nohup ${CLIENT_EXE} ${N_WORKER} 0 40017 ${SERVER_IP} 40000 ${FORWARD_DIR} ${SUFFIX} > output/fclient1.out &
nohup ${CLIENT_EXE} ${N_WORKER} 1 40018 ${SERVER_IP} 40001 ${FORWARD_DIR} ${SUFFIX} > output/fclient2.out &
nohup ${CLIENT_EXE} ${N_WORKER} 2 40019 ${SERVER_IP} 40002 ${FORWARD_DIR} ${SUFFIX} > output/fclient3.out &

# upload as server
nohup ${SERVER_EXE} 40017 ${FORWARD_DIR} > output/fserver1.out &
nohup ${SERVER_EXE} 40018 ${FORWARD_DIR} > output/fserver2.out &
nohup ${SERVER_EXE} 40019 ${FORWARD_DIR} > output/fserver3.out &