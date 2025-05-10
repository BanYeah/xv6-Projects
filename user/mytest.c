#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#define SIZE 20480

int main()
{
  printf("=== TEST START ===\n");

  printf("freemem: %d\n", freemem());

  char *file = (char *)mmap(8192, SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
  printf("freemem: %d\n", freemem());

  file[12288] = 'U';
  file[8192] = 'K';
  file[4096] = 'K';
  file[0] = 'S';

  for (int i = 0; i < SIZE; i++) {
    char buf[2] = {file[i], '\0'};
    printf("%s", buf);
  }
  printf("\n");

  printf("freemem: %d\n\n", freemem());


  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(0);
  }
  else if (pid == 0) {
    printf("In child process\n");
    printf("freemem: %d\n", freemem());

    file[16384] = '!';

    for (int i = 0; i < SIZE; i++) {
      char buf[2] = {file[i], '\0'};
      printf("%s", buf);
    }
    printf("\n\n");

    munmap((uint64)file - MMAPBASE);
    exit(0);
  }

  waitpid(pid);

  printf("In parent process\n");
  printf("freemem: %d\n", freemem());

  file[16385] = '?';

  for (int i = 0; i < SIZE; i++) {
    char buf[2] = {file[i], '\0'};
    printf("%s", buf);
  }
  printf("\n");

  // munmap((uint64)file - MMAPBASE); // 프로세스 종료 시 자동으로 munmap해야
  printf("freemem: %d\n", freemem());

  exit(0);
}

/* 실행 결과
=== TEST START ===
freemem: 32532
freemem: 32532
SKKU
freemem: 32525

In child process
freemem: 32508
SKKU!

In parent process
freemem: 32525
SKKU?
freemem: 32525
 */