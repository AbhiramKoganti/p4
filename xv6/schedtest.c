#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "param.h"
#include "pstat.h"

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

  if(first_loop[0] || second_loop[0]);
  int pidA = fork2(sliceA);
  if (!pidA) {
     exec(first_loop[0], first_loop);
     printf(1,"Didn't execute");
     exit();
  }

  int pidB = fork2(sliceB);
  if (!pidB) {
    exec(second_loop[0], second_loop);
    printf(1, "Didn't execute");
    exit();
  }
  
  sleep(sleepParent);

  struct pstat process_stats;
  int output=getpinfo(&process_stats);
  printf(1,"%d",output);
  int aindex = -1;
  int bindex = -1;

  int nprocs = sizeof process_stats.pid / sizeof nprocs;

  for (int i = 0; i < nprocs; i++) {
    if (process_stats.pid[i] == pidA) {
      aindex = i;
    }
    if (process_stats.pid[i] == pidB){
      bindex = i;
    }
    if (aindex + 1 && bindex + 1)
      break;
  }
  printf(1, "%d %d\n", process_stats.compticks[aindex], process_stats.compticks[bindex]); 
 exit();
}
