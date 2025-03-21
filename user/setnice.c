#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("setnice [PID] [VALUE]\n");
        return 0;
    }

    int res = setnice(atoi(argv[1]), atoi(argv[2]));
    if (res < 0)
        printf("error:\n");
}