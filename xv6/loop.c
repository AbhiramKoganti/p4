#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"


int main(int argc, char* argv[]){
  sleep(atoi(argv[1]));
  int i = 0, j = 0;
  while (i < 800000000) {
    j += i * j + 1;
    i++;
  }
  exit();
}
