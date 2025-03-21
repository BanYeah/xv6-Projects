#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int pid1 = fork();
    if (pid1 < 0) {
        printf("fork error\n");
        return 0;
    }
    else if (pid1 == 0)
        exit(0);
    
    int pid2 = fork();
    if (pid2 < 0) {
        printf("fork error\n");
        return 0;
    }
    else if (pid2 == 0)
        exit(0);

    ps(0);

    printf("waitpid(%d)\n", pid2);
    int res = waitpid(pid2);
    if (res < 0) {
        printf("waitpid error\n");
        return 0;
    }

    ps(0);

    printf("waitpid(%d)\n", pid1);
    res = waitpid(pid1);
    if (res < 0) {
        printf("waitpid error\n");
        return 0;
    }

    ps(0);

    res = waitpid(1);
    if (res < 0) {
        printf("waitpid error\n");
        return 0;
    }
}