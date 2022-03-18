#!/bin/bash

######## VARIABLES ######################

UPLOAD_DIR=/data/team-10/test_duplex/test1_compress
DOWNLOAD_DIR=/data/team-10/test_duplex/test2_compress
LISTEN_PORT_1=12340
LISTEN_PORT_2=12341
LISTEN_PORT_3=12342
LISTEN_PORT_4=12343
LISTEN_PORT_5=12344
LISTEN_PORT_6=12345

####### END ############################

N_WORKER_A=3
N_WORKER_B=3
REQ_ID_START_A=0
REQ_ID_END_A=250
REQ_ID_START_B=250
REQ_ID_END_B=500

# executable file
EXE=server/build/file_server

PREFIX=stock
SUFFIX=.zst


pkill -f file_server

rm -rf output/
mkdir -p output/

bash ./build-server.sh

mkdir -p ${UPLOAD_DIR}
mkdir -p ${DOWNLOAD_DIR}

# for forwarder A
nohup ${EXE} ${LISTEN_PORT_1} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_A} 0 ${REQ_ID_START_A} ${REQ_ID_END_A} ${PREFIX} ${SUFFIX} > output/server1.out &
nohup ${EXE} ${LISTEN_PORT_2} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_A} 1 ${REQ_ID_START_A} ${REQ_ID_END_A} ${PREFIX} ${SUFFIX} > output/server2.out &
nohup ${EXE} ${LISTEN_PORT_3} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_A} 2 ${REQ_ID_START_A} ${REQ_ID_END_A} ${PREFIX} ${SUFFIX} > output/server3.out &

# for forwarder B
nohup ${EXE} ${LISTEN_PORT_4} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_B} 0 ${REQ_ID_START_B} ${REQ_ID_END_B} ${PREFIX} ${SUFFIX} > output/server4.out &
nohup ${EXE} ${LISTEN_PORT_5} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_B} 1 ${REQ_ID_START_B} ${REQ_ID_END_B} ${PREFIX} ${SUFFIX} > output/server5.out &
nohup ${EXE} ${LISTEN_PORT_6} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${N_WORKER_B} 2 ${REQ_ID_START_B} ${REQ_ID_END_B} ${PREFIX} ${SUFFIX} > output/server6.out &