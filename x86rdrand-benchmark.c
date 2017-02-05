/*
 * Copyright (C) 2010-2017 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define STACK_SIZE	(65536)
#define ITERATIONS	(320000000)

#if !(defined(__x86_64__) || \
      defined(__x86_64) || \
      defined(__i386__) || \
      defined(__i386))
#error only for Intel processors!
#endif

#define cpuid(in, eax, ebx, ecx, edx)   \
  asm("cpuid":  "=a" (eax),             \
                "=b" (ebx),             \
                "=c" (ecx),             \
                "=d" (edx) : "a" (in))



#if defined(__x86_64__) || defined(__x86_64)
#define WIDTH	64
#else
#define WIDTH	32
#endif

#if defined(__x86_64__) || defined(__x86_64)
static inline uint64_t rdrand64(void)
{
        uint64_t        ret;

        asm volatile("1:;\n\
        rdrand %0;\n\
        jnc 1b;\n":"=r"(ret));

        return ret;
}
#else
static inline uint32_t rdrand32(void)
{
        uint32_t        ret;

        asm volatile("1:;\n\
        rdrand %0;\n\
        jnc 1b;\n":"=r"(ret));

        return ret;
}
#endif


#define RDRAND64x4	\
	rdrand64();	\
	rdrand64();	\
	rdrand64();	\
	rdrand64();	

#define RDRAND64x16	\
	RDRAND64x4	\
	RDRAND64x4	\
	RDRAND64x4	\
	RDRAND64x4	

#define RDRAND32x4	\
	rdrand32();	\
	rdrand32();	\
	rdrand32();	\
	rdrand32();	

#define RDRAND32x16	\
	RDRAND32x4	\
	RDRAND32x4	\
	RDRAND32x4	\
	RDRAND32x4

typedef struct {
	uint64_t usec;		/* duration of test in microseconds */
	uint64_t iter;		/* number of iterations taken */
	uint16_t instance;	/* instance number */
	pid_t	 pid;		/* clone process ID */
	char 	 stack[STACK_SIZE];/* stack */
} info_t;

static volatile bool start_test, end_test;

static int test64(void *private)
{
	struct timeval tv1, tv2;
	uint64_t usec1, usec2;
	info_t *const info = (info_t *)private;
	register uint64_t i;
	const uint64_t iter = info->iter / 32;

	end_test = false;
	while (!start_test)
		;

	if (info->instance == 0) {
		gettimeofday(&tv1, NULL);

		for (i = 0; i < iter; i++) {
#if defined(__x86_64__) || defined(__x86_64)
			RDRAND64x16
			RDRAND64x16
#else
			RDRAND32x16
			RDRAND32x16
#endif
		}

		end_test = true;
		gettimeofday(&tv2, NULL);

	} else {
		gettimeofday(&tv1, NULL);

		for (i = 0; i < iter; i++) {
#if defined(__x86_64__) || defined(__x86_64)
			RDRAND64x16
			RDRAND64x16
#else
			RDRAND32x16
			RDRAND32x16
#endif
		}

		gettimeofday(&tv2, NULL);
	}

	usec1 = (tv1.tv_sec * 1000000) + tv1.tv_usec;
	usec2 = (tv2.tv_sec * 1000000) + tv2.tv_usec;

	info->iter = i * 32;
	info->usec = usec2 - usec1;

	_exit(0);
}

static void test(const uint32_t threads)
{
	uint32_t i;
	uint64_t usec = 0, iter = 0;
	info_t *info;
	double nsec;

	info = (info_t *)calloc(threads, sizeof(info_t));
	if (!info) {
		fprintf(stderr, "Failed to allocate thread state and stack\n");
		exit(1);
	}

	start_test = false;

	for (i = 0; i < threads; i++) {
		info[i].instance = i;
		info[i].iter = ITERATIONS;
		info[i].pid = clone(test64, &info[i].stack[STACK_SIZE - 64],
			CLONE_VM | SIGCHLD, &info[i]);
	}

	start_test = true;

	for (i = 0; i < threads; i++) {
		int status, ret;

		ret = waitpid(info[i].pid, &status, 0);
		(void)ret;
	}

	for (i = 0; i < threads; i++) {
		usec += info[i].usec;
		iter += info[i].iter;
	}
	usec /= threads;
	nsec = 1000.0 * (double)usec / iter;

	printf("%" PRIu16 "\t%8.3f\t%8.3f\t  %12.7f\n",
		threads, nsec, 1000.0 / nsec, (float)WIDTH / nsec);
}

int main(void)
{
	uint32_t i, eax, ebx, ecx, edx = 0;
	const uint32_t cpus = sysconf(_SC_NPROCESSORS_ONLN);

	/* Intel CPU? */
	cpuid(0, eax, ebx, ecx, edx);
	if (!((memcmp(&ebx, "Genu", 4) == 0) &&
	      (memcmp(&edx, "ineI", 4) == 0) &&
	      (memcmp(&ecx, "ntel", 4) == 0))) {
		fprintf(stderr, "Not a recognised Intel CPU.\n");
		exit(EXIT_FAILURE);
	}
	/* ..and supports rdrand? */
	cpuid(1, eax, ebx, ecx, edx);
	if (!(ecx & 0x40000000)) {
		fprintf(stderr, "CPU does not support rdrand.\n");
		exit(EXIT_FAILURE);
	}

	printf("Exercising %d bit rdrands:\n", WIDTH);
	printf("Threads\trdrand\t\tmillion rdrands\t  billion bits\n");
	printf("\tduration (ns)\tper second\t  per second\n");
	for (i = 1; i <= cpus; i++)
		test(i);

	exit(EXIT_SUCCESS);
}
