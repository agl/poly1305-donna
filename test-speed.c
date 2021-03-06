#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "poly1305-donna.h"
#include "portable-jane.h"

/* ticks - not tested on anything other than x86 */
static uint64_t
get_ticks(void) {
#if defined(CPU_X86) || defined(CPU_X86_64)
	#if defined(COMPILER_INTEL)
		return _rdtsc();
	#elif defined(COMPILER_MSVC)
		return __rdtsc();
	#elif defined(COMPILER_GCC)
		uint32_t lo, hi;
		__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
		return ((uint64_t)lo | ((uint64_t)hi << 32));
	#else
		need rdtsc for this compiler
	#endif
#elif defined(OS_SOLARIS)
	return (uint64_t)gethrtime();
#elif defined(CPU_SPARC) && !defined(OS_OPENBSD)
	uint64_t t;
	__asm__ __volatile__("rd %%tick, %0" : "=r" (t));
	return t;
#elif defined(CPU_PPC)
	uint32_t lo = 0, hi = 0;
	__asm__ __volatile__("mftbu %0; mftb %1" : "=r" (hi), "=r" (lo));
	return ((uint64_t)lo | ((uint64_t)hi << 32));
#elif defined(CPU_IA64)
	uint64_t t;
	__asm__ __volatile__("mov %0=ar.itc" : "=r" (t));
	return t;
#elif defined(OS_NIX)
	timeval t2;
	gettimeofday(&t2, NULL);
	t = ((uint64_t)t2.tv_usec << 32) | (uint64_t)t2.tv_sec;
	return t;
#else
	need ticks for this platform
#endif
}

#define timeit(x,minvar) {       \
	ticks = get_ticks();         \
 	x;                           \
	ticks = get_ticks() - ticks; \
	if (ticks < minvar)          \
		minvar = ticks;          \
	}

#define maxticks 0xffffffffffffffffull


int main() {
	static size_t lengths[] = {16, 64, 256, 1024, 8192, 0};
	static unsigned char buf[8192] = {255};
	unsigned char key[32] = {127}, mac[16];
	size_t i, j;
	uint64_t ticks, minticks;

	/* warm up */
	for (i = 0; i < 65536; i++) {
		poly1305_auth(mac, buf, 8192, key);
		buf[i & 8191] += mac[i & 15];
	}

	for (i = 0; lengths[i]; i++) {
		minticks = maxticks;
		for (j = 0; j < 32768; j++) {
			timeit(poly1305_auth(mac, buf, lengths[i], key), minticks);
			buf[j & 8191] += mac[i & 15];
		}
		if (lengths[i] <= 256)
			printf("%u bytes, %.0f cycles\n", (uint32_t)lengths[i], (double)minticks);
		else
			printf("%u bytes, %.2f cycles/byte\n", (uint32_t)lengths[i], (double)minticks / lengths[i]);
	}

	return 0;
}