#!/bin/bash

pkill -f file_client

bash build-client.sh

client/build/file_client 1 0 12000 10.216.68.191 11999 /data/team-10/test_duplex/test2_compress /data/team-10/test_duplex/test1_compress 0 500 stock .zst