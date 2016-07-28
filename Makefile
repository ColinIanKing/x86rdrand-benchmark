BINDIR=/usr/bin

x86rdrand-benchmark: x86rdrand-benchmark.o
	$(CC) $< -o $@ -lpthread

CFLAGS += -O3 -Wall -Werror

clean:
	rm -f x86rdrand-benchmark x86rdrand-benchmark.o

install: x86rdrand-benchmark
	mkdir -p ${DESTDIR}${BINDIR}
	cp x86rdrand-benchmark ${DESTDIR}${BINDIR}

