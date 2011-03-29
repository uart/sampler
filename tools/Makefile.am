bin_PROGRAMS = usfsampler

CPPFLAGS = -I $(top_srcdir)/include

usfsampler_SOURCES =				\
	usfsampler.c

usfsampler_LDADD = ../lib/libusampler.a -lusf -lbz2 -lm
