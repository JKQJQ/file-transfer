#!/bin/bash

######## VARIABLES ######################

# exchange specific
REQ_ID_START=0
REQ_ID_END=250

# connection to trader 1
SERVER_IP_1=10.216.68.191
SERVER_IP_2=${SERVER_IP_1}
SERVER_IP_3=${SERVER_IP_1}
SERVER_PORT_1=12340
SERVER_PORT_2=12341
SERVER_PORT_3=12342

# connection to trader 2
SERVER_IP_4=10.216.68.190
SERVER_IP_5=10.216.68.190
SERVER_IP_6=10.216.68.190
SERVER_PORT_4=12340
SERVER_PORT_5=12341
SERVER_PORT_6=12342

# LOCAL PORTS
LOCAL_PORT_1=12340
LOCAL_PORT_2=12341
LOCAL_PORT_3=12342
LOCAL_PORT_4=12343
LOCAL_PORT_5=12344
LOCAL_PORT_6=12345


DOWNLOAD_DIR=/data/team-10/forward/download
UPLOAD_DIR=/data/team-10/forward/upload

####### END ############################

pkill -f file_client

rm -rf output/
mkdir -p output/

bash ./build-client.sh

EXE=client/build/file_client
N_WORKER=3
PREFIX=stock
SUFFIX=.zst

mkdir -p ${DOWNLOAD_DIR}
mkdir -p ${UPLOAD_DIR}

nohup ${EXE} ${N_WORKER} 0 ${SERVER_PORT_1} ${SERVER_IP_1} ${LOCAL_PORT_1} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client1.out &
nohup ${EXE} ${N_WORKER} 1 ${SERVER_PORT_2} ${SERVER_IP_2} ${LOCAL_PORT_2} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client2.out &
nohup ${EXE} ${N_WORKER} 2 ${SERVER_PORT_3} ${SERVER_IP_3} ${LOCAL_PORT_3} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client3.out &
nohup ${EXE} ${N_WORKER} 0 ${SERVER_PORT_4} ${SERVER_IP_4} ${LOCAL_PORT_4} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client4.out &
nohup ${EXE} ${N_WORKER} 1 ${SERVER_PORT_5} ${SERVER_IP_5} ${LOCAL_PORT_5} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client5.out &
nohup ${EXE} ${N_WORKER} 2 ${SERVER_PORT_6} ${SERVER_IP_6} ${LOCAL_PORT_6} ${UPLOAD_DIR} ${DOWNLOAD_DIR} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client6.out &