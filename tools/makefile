#
# Portos v1.7.0
# Copyright (c) 2003-2014 by Nestwave SAS. All rights reserved.
#

TARGET     := sim
VERSION    := release

DIR_TOP    := ..

CFILES     := replace_strings.c mygrep.c po_cmd.c
ALLFILES   := $(wildcard *.c *.h)

EXECS      := $(CFILES:.c=.exe)

include $(DIR_TOP)/makefile.def

all: depend  $(EXECS)  $(DIR_TOP)/bin/po_preprocess.exe

$(DIR_TOP)/bin/po_preprocess.exe: po_preprocess.c
	c:/MinGW/bin/gcc -O3 -Wall po_preprocess.c -o $(DIR_TOP)/bin/po_preprocess.exe

%.exe:	%.c
	gcc -O3 -Wall $*.c -o $@

# Put this one last
include $(DIR_TOP)/makefile.inc
