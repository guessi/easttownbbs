#!/bin/sh

find /home/bbs/usr/@/ -type d -maxdepth 1 -mtime +240 -exec rm -rf {} \;
