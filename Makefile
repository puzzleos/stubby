#
# stubby makefile
#
# Copyright (c) 2020 Cisco Systems, Inc. <pmoore2@cisco.com>
#

#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

include make.conf

STUBBY_OBJS = \
	util.o \
	disk.o \
	pe.o \
	linux.o \
	stub.o \
	stra.o \
	kcmdline.o \

TEST_EXES = test-cmdline test-get-cmdline
TEST_OBJS = linux_efilib-lt.o kcmdline-lt.o stra-lt.o

.PHONY: all
all: build test

.PHONY: build
build: stubby.efi $(TEST_EXES)

stubby.so: ${STUBBY_OBJS}
	${LD} ${LDFLAGS} $^ -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-${ARCH} $^ $@


.PHONY: test
test: $(TEST_EXES)
	@for t in $(TEST_EXES); do echo -- $$t --; ./$$t || exit; done

LINUX_TEST_CFLAGS := -DLINUX_TEST $(filter-out -fshort-wchar,$(CFLAGS))
%-lt.o: %.c
	$(CC) $(LINUX_TEST_CFLAGS) -c -o $@ $^

test-%: test-%-lt.o $(TEST_OBJS)
	$(CC) $(LINUX_TEST_CFLAGS) -o $@ $^

.PHONY: show-version
show-version:
	@echo $(VERSION)

.PHONY: clean
tarball:
	@[ -s git-version ] || echo $(VERSION)
	./tools/make-tarball

.PHONY: clean
clean:
	${RM} -rf *.efi *.so *.o $(TEST_EXES)

.PHONY: github-workflow
github-workflow:
	run-parts --verbose tools/workflow.d

# kate: syntax Makefile;
