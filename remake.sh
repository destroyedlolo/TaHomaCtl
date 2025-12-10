#!/bin/bash
# This script will rebuild a Makefile suitable to compile TaHomaCtl

LFMakeMaker -v +f=Makefile -cc='cc -Wall -pedantic -O2' --opts='-lreadline -lhistory $(shell pkg-config --cflags --libs avahi-client libcurl json-c) -lrt' *.c -t=TaHomaCtl > Makefile
