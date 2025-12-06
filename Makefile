# makefile created automaticaly by LFMakeMaker
# LFMakeMaker 1.6 (Apr 26 2025 11:57:56) (c)LFSoft 1997

gotoall: all


#The compiler (may be customized for compiler's options).
cc=cc -Wall -pedantic -O2
opts=-lreadline -lhistory $(shell pkg-config --cflags --libs avahi-client libcurl)

APIrequest.o : APIrequest.c TaHomaCtl.h Makefile 
	$(cc) -c -o APIrequest.o APIrequest.c $(opts) 

AvahiScaning.o : AvahiScaning.c TaHomaCtl.h Makefile 
	$(cc) -c -o AvahiScaning.o AvahiScaning.c $(opts) 

TaHomaCtl.o : TaHomaCtl.c TaHomaCtl.h Makefile 
	$(cc) -c -o TaHomaCtl.o TaHomaCtl.c $(opts) 

TaHomaCtl : TaHomaCtl.o AvahiScaning.o APIrequest.o Makefile 
	 $(cc) -o TaHomaCtl TaHomaCtl.o AvahiScaning.o APIrequest.o \
  $(opts) 

all: TaHomaCtl 
