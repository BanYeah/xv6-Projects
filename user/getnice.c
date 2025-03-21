#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("getnice [PID]\n");
        return 0;
    }

    int nice = getnice(atoi(argv[1]));
    if (nice < 0)
        printf("error: no corresponding process!\n");
    else
        printf("%d\n", nice);
}