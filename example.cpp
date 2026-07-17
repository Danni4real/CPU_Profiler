#include <cmath>
#include <sys/mman.h>

#include "profiler.h"

double cpu_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "cpu_heavy\n";

    double x{};
    static int i{};

    for (int c=0;c<1000;c++) {
        i++;

        x += sin(i) * cos(sqrt(i + 1));
        x = fmod(x, 1000.0);
    }

    return x;
}

void syscall_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "syscall_heavy\n";

    timespec ts{};
    for (int c=0;c<1000;c++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }
}

void mmap_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "mmap_heavy\n";

    const size_t sz = 4096 * 10;

    for (int c=0;c<1000;c++) {
        void* p = mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p != MAP_FAILED) {
            munmap(p, sz);
        }
    }
}

int main() {

    while (true) {
        cpu_heavy();
        syscall_heavy();
        mmap_heavy();
    }

    return 0;
}
