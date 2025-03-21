#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ps [PID]\n");
        return 0;
    }

    ps(atoi(argv[1]));
}