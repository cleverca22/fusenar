CPPFLAGS=-D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -g -std=c++11 -Wall -Werror
CFLAGS=-D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -g -Wall -Werror

fusenar: fusenar.o nar.o fusenar_core.o
	g++ -g -o $@ $+ -lfuse -lnixstore

install: fusenar
	mkdir -pv ${out}/bin/
	cp -vi fusenar ${out}/bin/

%.o: %.c
	gcc ${CFLAGS} -o $@ -c $<
%.o: %.cpp
	g++ ${CPPFLAGS} -o $@ -c $<
