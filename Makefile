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

# Environment variables
PREFIX=/usr/local
CC=gcc
CFLAGS=-c -Wall -D_REENTRANT -I$(PREFIX)/include $(ADDITIONALFLAGS)
LIBS=-L$(PREFIX)/lib -lc -ldl -ljansson -lulfius -lhoel -lyder -lorcania
MODULES_LOCATION=service-modules

carleon-standalone: carleon.o service.o profile.o carleon-standalone.o
	$(CC) -o carleon-standalone carleon-standalone.o carleon.o service.o profile.o $(LIBS) -lconfig

carleon-standalone.o: carleon-standalone.c carleon.h
	$(CC) $(CFLAGS) carleon-standalone.c

carleon.o: carleon.c carleon.h
	$(CC) $(CFLAGS) carleon.c

service.o: service.c carleon.h
	$(CC) $(CFLAGS) service.c

profile.o: profile.c carleon.h
	$(CC) $(CFLAGS) profile.c

modules:
	cd $(MODULES_LOCATION) && $(MAKE) debug

all: release

debug: ADDITIONALFLAGS=-DDEBUG -g -O0

debug: carleon-standalone modules unit-tests

release-standalone: ADDITIONALFLAGS=-O3

release-standalone: carleon-standalone modules

release: ADDITIONALFLAGS=-O3

release: carleon.o device.o profile.o modules

test: debug
	./carleon-standalone

clean:
	rm -f *.o carleon-standalone unit-tests
	cd $(MODULES_LOCATION) && $(MAKE) clean

unit-tests: unit-tests.c
	$(CC) -o unit-tests unit-tests.c -lc -lulfius -lorcania -ljansson -L$(PREFIX)/lib

install_modules: modules
	cd $(MODULES_LOCATION) && $(MAKE) install
