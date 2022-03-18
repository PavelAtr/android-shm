#!/bin/bash

LD_LIBRARY_PATH=./ \
./shm-launch "LD_PRELOAD=libandroid-shm.so ./test-server 1 2 3" "LD_PRELOAD=libandroid-shm.so ./test-client 4 5 6"
