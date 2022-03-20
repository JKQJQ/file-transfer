#!/bin/bash

######## VARIABLES ######################

# exchange specific
REQ_ID_START=250
REQ_ID_END=500

# Trade1->Exchange2
# Trade1
SERVER_PORT_1=59000
SERVER_PORT_2=59010
SERVER_PORT_3=59020
# Exchange2
LOCAL_PORT_1=58000
LOCAL_PORT_2=57090
LOCAL_PORT_3=57080

# Trade2->Exchange2
# Trade2
SERVER_PORT_4=55055
SERVER_PORT_5=55054
SERVER_PORT_6=55053
# Exchange2
LOCAL_PORT_4=50123
LOCAL_PORT_5=50124
LOCAL_PORT_6=50125

# connection to trader 1
SERVER_IP_1=10.216.68.217
SERVER_IP_2=${SERVER_IP_1}
SERVER_IP_3=${SERVER_IP_1}

# connection to trader 2
SERVER_IP_4=10.216.68.218
SERVER_IP_5=${SERVER_IP_4}
SERVER_IP_6=${SERVER_IP_4}

FORWARD_DIR_A=/data/team-10/forward/from_trader_1
FORWARD_DIR_B=/data/team-10/forward/from_trader_2

####### END ############################

pkill -f file_client

rm -rf output/
mkdir -p output/

bash ./build-client.sh

EXE=client/build/file_client
N_WORKER=3
PREFIX=stock
SUFFIX=.zst

mkdir -p ${FORWARD_DIR_A}
mkdir -p ${FORWARD_DIR_B}

# trader 1
nohup ${EXE} ${N_WORKER} 0 ${SERVER_PORT_1} ${SERVER_IP_1} ${LOCAL_PORT_1} ${FORWARD_DIR_A} ${FORWARD_DIR_B} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client1.out &
nohup ${EXE} ${N_WORKER} 1 ${SERVER_PORT_2} ${SERVER_IP_2} ${LOCAL_PORT_2} ${FORWARD_DIR_A} ${FORWARD_DIR_B} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client2.out &
nohup ${EXE} ${N_WORKER} 2 ${SERVER_PORT_3} ${SERVER_IP_3} ${LOCAL_PORT_3} ${FORWARD_DIR_A} ${FORWARD_DIR_B} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client3.out &

# trader 2
nohup ${EXE} ${N_WORKER} 0 ${SERVER_PORT_4} ${SERVER_IP_4} ${LOCAL_PORT_4} ${FORWARD_DIR_B} ${FORWARD_DIR_A} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client4.out &
nohup ${EXE} ${N_WORKER} 1 ${SERVER_PORT_5} ${SERVER_IP_5} ${LOCAL_PORT_5} ${FORWARD_DIR_B} ${FORWARD_DIR_A} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client5.out &
nohup ${EXE} ${N_WORKER} 2 ${SERVER_PORT_6} ${SERVER_IP_6} ${LOCAL_PORT_6} ${FORWARD_DIR_B} ${FORWARD_DIR_A} ${REQ_ID_START} ${REQ_ID_END} ${PREFIX} ${SUFFIX} > output/client6.out &