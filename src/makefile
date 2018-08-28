#
# Portos v1.7.0
# Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
#

DIR_TOP    := ..

CFILES     := $(wildcard *.c)
ALLFILES   := $(wildcard * target_*/* todo/*)

include $(DIR_TOP)/makefile.def

all: depend  $(OBJS) $(DIR_LIB)/portos.a

# PORTOS library
$(PORTOS_LIB): $(OBJS)
	mkdir -p $(DIR_LIB)
	$(AR) rvu $@ $(OBJS)
	ranlib $@

# Put this one last
include $(DIR_TOP)/makefile.inc

# DO NOT DELETE
