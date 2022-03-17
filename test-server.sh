#!/bin/bash
pkill -f file_server

bash build-server.sh
server/build/file_server 12000 /data/team-10/test_duplex/test1_compress /data/team-10/test_duplex/test2_compress 1 0 0 500 stock .zst