#                                                              -*- Makefile -*-
# Copyright (c) Virtutech AB, All Rights Reserved
#
# Simics module makefile
#

MODULE_CLASSES = uart-sampler	\
		 uart-sampler-conf

SRC_FILES = uart-sampler.c	\
	    uart-sampler-conf.c \
	    hash.c	        \
	    sampler.c

LIBS = -lusf -lm

MODULE_CFLAGS = -I$(SIMICS_WORKSPACE)/modules/uart-sampler/lib/include

EXTRA_VPATH = $(SIMICS_WORKSPACE)/modules/uart-sampler/sampler \
              $(SIMICS_WORKSPACE)/modules/uart-sampler/lib

include $(MODULE_MAKEFILE)
