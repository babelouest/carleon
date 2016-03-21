#
# Carleon House Automation side services
#
# Command house automation side services via an HTTP REST interface
#
# Makefile used to build the software
#
# Copyright 2016 Nicolas Mora <mail@babelouest.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU GENERAL PUBLIC LICENSE
# License as published by the Free Software Foundation;
# version 3 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU GENERAL PUBLIC LICENSE for more details.
#
# You should have received a copy of the GNU General Public
# License along with this library.  If not, see <http://www.gnu.org/licenses/>.
#

PREFIX=/usr/local
CC=gcc
CFLAGS=-c -fPIC -Wall -D_REENTRANT -shared -nostartfiles -I$(PREFIX)/include $(ADDITIONALFLAGS)
LIBS=-L$(PREFIX)/lib -lc -ljansson -lyder -lorcania -lulfius
VERSION=0.1.0

all: release

libservicemock.so: service-mock.o
	$(CC) -shared -Wl,-soname,libservicemock.so -o libservicemock.so service-mock.o $(LIBS)

device-mock.o: service-mock.c
	$(CC) $(CFLAGS) $(FLAGS_YDER) service-mock.c

clean:
	rm -f *.o *.so

debug: ADDITIONALFLAGS=-DDEBUG -g -O0

debug: libservicemock.so

release: ADDITIONALFLAGS=-O3

release: libservicemock.so
