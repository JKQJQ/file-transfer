#!/bin/bash
rm -rf output/
mkdir output/

nohup client/build/file_client 0 40017 10.216.68.190 12345 /data/team-10/multiprocess-remote-test-limit/  > output/client1.out &
nohup client/build/file_client 1 40018 10.216.68.190 12344 /data/team-10/multiprocess-remote-test-limit/  > output/client2.out &
nohup client/build/file_client 2 40019 10.216.68.190 12343 /data/team-10/multiprocess-remote-test-limit/  > output/client3.out &

