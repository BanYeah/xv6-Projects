#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    uint64 freemem = meminfo();
    printf("freemem: %lu bytes\n", freemem);
}