#
# Copyright (C) 2010-2021 Canonical, Ltd.
# Copyright (C) 2021-2025 Colin Ian King.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

BINDIR=/usr/bin

x86rdrand-benchmark: x86rdrand-benchmark.o
	$(CC) $< -o $@
	strip $@

CFLAGS += -O3 -Wall -Werror -Wextra -fipa-pta -ftree-vectorize -fweb -fwhole-program -fivopts

#
# Pedantic flags
#
ifeq ($(PEDANTIC),1)
CFLAGS += -Wabi -Wcast-qual -Wfloat-equal -Wmissing-declarations \
        -Wmissing-format-attribute -Wno-long-long -Wpacked \
        -Wredundant-decls -Wshadow -Wno-missing-field-initializers \
        -Wno-missing-braces -Wno-sign-compare -Wno-multichar
endif


clean:
	rm -f x86rdrand-benchmark x86rdrand-benchmark.o

install: x86rdrand-benchmark
	mkdir -p ${DESTDIR}${BINDIR}
	cp x86rdrand-benchmark ${DESTDIR}${BINDIR}

