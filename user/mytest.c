#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
  printf("========== Testing getnice() ==========\n");
  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  printf("$ getnice 1\n");
  printf("%d\n\n", getnice(1));

  printf("$ getnice 12\n");
  printf("%d\n\n", getnice(12)); // error


  printf("========== Testing setnice() ==========\n");
  printf("$ setnice 3 10\n");
  printf("%d\n\n", setnice(3, 10));

  printf("$ setnice 12 10\n");
  printf("%d\n\n", setnice(12, 10)); // error

  printf("$ setnice 1 40\n");
  printf("%d\n\n", setnice(1, 40)); // error


  printf("========== Testing ps() ==========\n");
  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  printf("$ ps 1\n");
  ps(1);
  printf("\n");

  printf("$ ps 12\n");
  ps(12);
  printf("\n");


  printf("========== Testing meminfo() ==========\n");
  printf("$ meminfo\n");
  printf("%lu\n\n", meminfo());


  printf("========== Testing waitpid() ==========\n");
  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  int pid1 = fork();
  if (pid1 < 0) {
    printf("fork error\n");
    return 0;
  }
  else if (pid1 == 0) // child
    exit(0);

  int pid2 = fork();
  if (pid2 < 0) {
    printf("fork error\n");
    return 0;
  }
  else if (pid2 == 0) // child
    exit(0);

  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  printf("$ meminfo\n");
  printf("%lu\n\n", meminfo());

  printf("$ waitpid %d\n", pid2);
  printf("%d\n\n", waitpid(pid2));

  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  printf("$ waitpid %d\n", pid1);
  printf("%d\n\n", waitpid(pid1));

  printf("$ ps 0\n");
  ps(0);
  printf("\n");

  printf("$ waitpid 1\n");
  printf("%d\n\n", waitpid(1)); // error
}
