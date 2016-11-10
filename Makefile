# Makefile for hosts2zones
#
# Created by Dr. Rolf Jansen on 2014-08-02.
# Copyright (c) 2014 projectworld.net. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


CC     ?= clang
CFLAGS  = $(CDEFS) -std=c11 -g0 -Ofast -mssse3 -fstrict-aliasing -ffast-math -Wno-parentheses
PREFIX ?= /usr/local

HEADERS = binutils.h store.h
SOURCES = binutils.c store.c hosts2zones.c
OBJECTS = $(SOURCES:.c=.o)
PRODUCT = hosts2zones

all: $(HEADERS) $(SOURCES) $(OBJECTS) $(PRODUCT)

depend:
	$(CC) $(CFLAGS) -E -MM *.c > .depend

$(OBJECTS): Makefile
	$(CC) $(CFLAGS) $< -c -o $@

$(PRODUCT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	rm -rf *.o *.core $(PRODUCT)

update: clean all

install: $(PRODUCT)
	install -m 555 -s ipup $(DESTDIR)${PREFIX}/bin/$(PRODUCT) $(PRODUCT)
