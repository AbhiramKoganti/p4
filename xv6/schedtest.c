#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"


int main(int argc, char* argv[]){
  char loop[5];
  strcpy(loop, "loop");
  
  char *first_loop[3];// = {"loop", argv[2], 0};
  first_loop[0] = loop;
  first_loop[1] = argv[2];
  
  char* second_loop[3];// = {"loop", argv[4], 0};
  second_loop[0] = loop; 
  second_loop[1] = argv[4];

  int sliceA = atoi(argv[1]);
  int sliceB = atoi(argv[3]);
  int sleepParent = atoi(argv[5]);

  fork2(sliceA);
  if (getpid() == 0) {
     exec(first_loop[0], first_loop);
     exit();
  }

  fork2(sliceB);
  if (getpid() == 0) {
    exec(second_loop[0], second_loop);
  }

  sleep(sleepParent);
}
