#!/bin/bash

pkill -f file_client
rm -rf output/
mkdir output/

bash ./build-client.sh

N_WORKER=3
SERVER_IP=10.216.68.189
DOWN_DIR=/data/team-10/zipped_files/
EXE=client/build/file_client
SUFFIX=.zst

nohup ${EXE} ${N_WORKER} 0 40017 ${SERVER_IP} 12345 ${DOWN_DIR} ${SUFFIX} > output/client1.out &
nohup ${EXE} ${N_WORKER} 1 40018 ${SERVER_IP} 12344 ${DOWN_DIR} ${SUFFIX} > output/client2.out &
nohup ${EXE} ${N_WORKER} 2 40019 ${SERVER_IP} 12342 ${DOWN_DIR} ${SUFFIX} > output/client3.out &
# nohup client/build/file_client 5 3 40020 10.216.68.190 12342 /data/team-10/multiprocess-remote-test-limit/  > output/client4.out &
# nohup client/build/file_client 5 4 40021 10.216.68.190 12341 /data/team-10/multiprocess-remote-test-limit/  > output/client5.out &

