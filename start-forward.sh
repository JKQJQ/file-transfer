#!/bin/bash

pkill -f file_client
pkill -f file_server
rm -rf output/
mkdir output/

bash ./build-server.sh
bash ./build-client.sh

/data/team-10/forward/

# download
nohup client/build/file_client 3 0 40017 10.216.68.190 40000 /data/team-10/forward/  > output/fclient1.out &
nohup client/build/file_client 3 1 40018 10.216.68.190 40001 /data/team-10/forward/  > output/fclient2.out &
nohup client/build/file_client 3 2 40019 10.216.68.190 40002 /data/team-10/forward/  > output/fclient3.out &

# upload
nohup server/build/file_server 40017 /data/team-10/forward/ > output/fserver1.out &
nohup server/build/file_server 40018 /data/team-10/forward/ > output/fserver2.out &
nohup server/build/file_server 40019 /data/team-10/forward/ > output/fserver3.out &