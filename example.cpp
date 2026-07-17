#include <cmath>
#include <unistd.h>
#include <sys/mman.h>

#include "profiler.h"

double cpu_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "cpu_heavy\n";

    double x{};
    static int i{};

    for (int c = 0; c < 1000; c++) {
        i++;

        x += sin(i) * cos(sqrt(i + 1));
        x = fmod(x, 1000.0);
    }

    usleep(1);

    return x;
}

void syscall_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "syscall_heavy\n";

    timespec ts{};
    for (int c = 0; c < 1000; c++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }

    usleep(1);
}

void mmap_heavy() {
    PROFILE_FUNCTION();
    // std::cout << "mmap_heavy\n";

    const size_t sz = 4096 * 10;

    for (int c = 0; c < 1000; c++) {
        void* p = mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p != MAP_FAILED) {
            munmap(p, sz);
        }
    }

    usleep(1);
}

int main() {
    std::thread([] {
        while (true) {
            cpu_heavy();
        }
    }).detach();

    std::thread([] {
        while (true) {
            syscall_heavy();
        }
    }).detach();

    std::thread([] {
        while (true) {
            mmap_heavy();
        }
    }).detach();

    while (true) {
        sleep(60);
    }

    return 0;
}
