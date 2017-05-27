ROOT = ../..
EMACS = $(ROOT)/src/emacs
CC      = gcc
LD      = gcc
LDFLAGS =
SO      = so
ifeq ($(SO),dll)
CFLAGS  = -std=gnu99 -ggdb3 -Wall
MECAB_INCLUDE =-I.
MECAB_LIB = libmecab.dll
else
CFLAGS  = -std=gnu99 -ggdb3 -Wall -fPIC
MECAB = /usr/local
MECAB_INCLUDE = -I$(MECAB)/include
MECAB_LIB = -L$(MECAB)/lib -lmecab
endif

all: reverse-translate-driver-mecab-module.$(SO)

%.$(SO): %.o
	$(LD) -shared $(LDFLAGS) -o $@ $< $(MECAB_LIB)

%.o: %.c
	$(CC) $(CFLAGS) -I. -I$(ROOT)/src $(MECAB_INCLUDE) -c $<
