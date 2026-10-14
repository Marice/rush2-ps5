/* Checks which memory model N64ModernRuntime can use in a PS5 title. Upstream
 * reserves 4 GiB of address space without access and makes the first 512 MiB
 * read/write (librecomp/src/recomp.cpp, addresses.hpp). Round 1 showed that an
 * anonymous PROT_NONE mmap of 1 GiB or more fails with ENOMEM, so this round
 * measures the budgets and tries MAP_GUARD, sceKernelReserveVirtualRange and
 * direct memory. Results go to /app0/memtest.txt. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <ps5platform/kernel.h>

#define GIB (1024ULL * 1024ULL * 1024ULL)
#define MIB (1024ULL * 1024ULL)

static FILE *out;

static void report(int ok, const char *what, long long code) {
    fprintf(out, "%s: %s", ok ? "PASS" : "FAIL", what);
    if (!ok) fprintf(out, " (code 0x%llx / errno %d)", (unsigned long long)code, errno);
    fputc('\n', out);
    fflush(out);
}

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void use(void *base, uint64_t size, const char *label) {
    char what[160];
    double t0 = now();
    memset(base, 0xA5, size);
    const uint8_t *p = base;
    int same = p[0] == 0xA5 && p[size / 2] == 0xA5 && p[size - 1] == 0xA5;
    snprintf(what, sizeof what, "%s: wrote and read %llu MiB in %.3f s", label,
             (unsigned long long)(size / MIB), now() - t0);
    report(same, what, 0);
}

static void try_mmap(uint64_t size, int prot, int flags, const char *label) {
    char what[160];
    errno = 0;
    void *base = mmap(NULL, size, prot, flags, -1, 0);
    snprintf(what, sizeof what, "%s (%llu MiB) at %p", label, (unsigned long long)(size / MIB),
             base == MAP_FAILED ? NULL : base);
    report(base != MAP_FAILED, what, (long long)(intptr_t)base);
    if (base != MAP_FAILED) munmap(base, size);
}

/* Reserve `reserve` bytes, then back the first `commit` bytes with direct memory
 * mapped at the start of the reservation, the way the runtime needs rdram. */
static void try_reserve_and_direct(uint64_t reserve, uint64_t commit) {
    char what[160];
    void *base = NULL;
    int32_t rc = sceKernelReserveVirtualRange(&base, reserve, 0, 64 * 1024);
    snprintf(what, sizeof what, "sceKernelReserveVirtualRange %llu MiB at %p",
             (unsigned long long)(reserve / MIB), base);
    report(rc == 0, what, rc);
    if (rc != 0) return;

    int64_t phys = 0;
    rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), commit,
                                       PS5_KERNEL_DIRECT_ALIGNMENT, PS5_KERNEL_DIRECT_TYPE_CPU, &phys);
    snprintf(what, sizeof what, "sceKernelAllocateDirectMemory %llu MiB", (unsigned long long)(commit / MIB));
    report(rc == 0, what, rc);
    if (rc != 0) { sceKernelMunmap(base, reserve); return; }

    void *at = base;
    rc = sceKernelMapDirectMemory(&at, commit, PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_WRITE,
                                  PS5_KERNEL_MAP_FIXED, phys, PS5_KERNEL_DIRECT_ALIGNMENT);
    snprintf(what, sizeof what, "sceKernelMapDirectMemory fixed at the reservation (%p)", at);
    report(rc == 0 && at == base, what, rc);
    if (rc == 0 && at == base) {
        use(base, commit, "reserved + direct");
        /* The byte after the mapped part must still fault-protect: query it. */
        void *s = NULL, *e = NULL;
        uint32_t prot = 0;
        int32_t q = sceKernelQueryMemoryProtection((uint8_t *)base + commit, &s, &e, &prot);
        fprintf(out, "      past the mapped part: query 0x%x, prot 0x%x\n", q, prot);
    }
    sceKernelMunmap(base, reserve);
    sceKernelReleaseDirectMemory(phys, commit);
}

int main(void) {
    out = fopen("/app0/memtest.txt", "w");
    if (!out) return 1;
    fprintf(out, "Rush 2 PS5 memory test, round 2\n");

    size_t flex_conf = 0, flex_avail = 0, direct_avail = 0;
    int64_t direct_start = 0;
    sceKernelConfiguredFlexibleMemorySize(&flex_conf);
    sceKernelAvailableFlexibleMemorySize(&flex_avail);
    sceKernelAvailableDirectMemorySize(0, sceKernelGetDirectMemorySize(), 64 * 1024, &direct_start, &direct_avail);
    fprintf(out, "flexible memory: configured %zu MiB, available %zu MiB\n", flex_conf / MIB, flex_avail / MIB);
    fprintf(out, "direct memory: total %lld MiB, largest free %zu MiB\n",
            (long long)(sceKernelGetDirectMemorySize() / (int64_t)MIB), direct_avail / MIB);

    try_mmap(64 * MIB, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, "anonymous read/write");
    try_mmap(512 * MIB, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, "anonymous read/write");
    try_mmap(64 * MIB, PROT_NONE, MAP_ANON | MAP_PRIVATE, "anonymous PROT_NONE");
    try_mmap(256 * MIB, PROT_NONE, MAP_ANON | MAP_PRIVATE, "anonymous PROT_NONE");
    try_mmap(4 * GIB, PROT_NONE, MAP_GUARD, "MAP_GUARD");

    try_reserve_and_direct(4 * GIB, 512 * MIB);
    try_reserve_and_direct(1 * GIB, 512 * MIB);

    fprintf(out, "done\n");
    fclose(out);
    return 0;
}
