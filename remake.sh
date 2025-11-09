#!/bin/bash
# This script will rebuild a Makefile suitable to compile TaHomaCtl

LFMakeMaker -v +f=Makefile -cc="cc -Wall -O2" *.c -t=TaHomaCtl > Makefile
