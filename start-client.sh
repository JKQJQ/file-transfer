#!/bin/bash

pkill -f file_client
rm -rf output/
mkdir output/

bash ./build-client.sh

nohup client/build/file_client 3 0 40017 10.216.68.191 12345 /data/team-10/forward-test/  > output/client1.out &
nohup client/build/file_client 3 1 40018 10.216.68.191 12344 /data/team-10/forward-test/  > output/client2.out &
nohup client/build/file_client 3 2 40019 10.216.68.191 12342 /data/team-10/forward-test/  > output/client3.out &
# nohup client/build/file_client 5 3 40020 10.216.68.190 12342 /data/team-10/multiprocess-remote-test-limit/  > output/client4.out &
# nohup client/build/file_client 5 4 40021 10.216.68.190 12341 /data/team-10/multiprocess-remote-test-limit/  > output/client5.out &

