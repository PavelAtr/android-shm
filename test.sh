#!/bin/bash

./shm-launch "LD_PRELOAD=libandroid-shm.so ./test-server" "LD_PRELOAD=libandroid-shm.so ./test-client"
