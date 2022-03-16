#!/bin/bash


######## VARIABLES ######################

SERVER_IP=10.216.68.192
DOWN_DIR=/data/team-10/test/order2/

####### END ############################

pkill -f file_client

rm -rf output/client*
mkdir -p output/

bash ./build-client.sh

N_WORKER=3
EXE=client/build/file_client
SUFFIX=.zst

mkdir -p ${DOWN_DIR}

nohup ${EXE} ${N_WORKER} 0 12350 ${SERVER_IP} 12353 ${DOWN_DIR} ${SUFFIX} > output/client1.out &
nohup ${EXE} ${N_WORKER} 1 12351 ${SERVER_IP} 12354 ${DOWN_DIR} ${SUFFIX} > output/client2.out &
nohup ${EXE} ${N_WORKER} 2 12352 ${SERVER_IP} 12355 ${DOWN_DIR} ${SUFFIX} > output/client3.out &
