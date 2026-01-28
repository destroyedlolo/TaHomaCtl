# makefile created automaticaly by LFMakeMaker
# LFMakeMaker 1.6 (Oct 23 2024 22:20:42) (c)LFSoft 1997

gotoall: all


#The compiler (may be customized for compiler's options).
cc=cc -Wall -pedantic -O2
opts=-lreadline -lhistory $(shell pkg-config --cflags --libs avahi-client libcurl json-c) -lrt

APIprocess.o : APIprocess.c TaHomaCtl.h Makefile 
	$(cc) -c -o APIprocess.o APIprocess.c $(opts) 

APIrequest.o : APIrequest.c TaHomaCtl.h Makefile 
	$(cc) -c -o APIrequest.o APIrequest.c $(opts) 

AvahiScaning.o : AvahiScaning.c TaHomaCtl.h Makefile 
	$(cc) -c -o AvahiScaning.o AvahiScaning.c $(opts) 

TaHomaCtl.o : TaHomaCtl.c TaHomaCtl.h Makefile 
	$(cc) -c -o TaHomaCtl.o TaHomaCtl.c $(opts) 

Utilities.o : Utilities.c TaHomaCtl.h Makefile 
	$(cc) -c -o Utilities.o Utilities.c $(opts) 

TaHomaCtl : Utilities.o TaHomaCtl.o AvahiScaning.o APIrequest.o \
  APIprocess.o Makefile 
	 $(cc) -o TaHomaCtl Utilities.o TaHomaCtl.o AvahiScaning.o \
  APIrequest.o APIprocess.o $(opts) 

all: TaHomaCtl 
