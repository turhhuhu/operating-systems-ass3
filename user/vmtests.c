#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#define REGION_SZ (4096)

void
sparse_memory(char *s)
{
  char *i, *prev_end, *new_end;
  
  prev_end = sbrk(REGION_SZ);
  if (prev_end == (char*)0xffffffffffffffffL) {
    printf("sbrk() failed\n");
    exit(1);
  }
  new_end = prev_end + REGION_SZ;

  for (i = prev_end + PGSIZE; i < new_end; i += 64 * PGSIZE)
    *(char **)i = i;

  for (i = prev_end + PGSIZE; i < new_end; i += 64 * PGSIZE) {
    if (*(char **)i != i) {
      printf("failed to read value from memory\n");
      exit(1);
    }
  }

  exit(0);
}

void
sparse_memory_unmap(char *s)
{
  int pid;
  char *i, *prev_end, *new_end;

  prev_end = sbrk(REGION_SZ);
  if (prev_end == (char*)0xffffffffffffffffL) {
    printf("sbrk() failed\n");
    exit(1);
  }
  new_end = prev_end + REGION_SZ;

  for (i = prev_end + PGSIZE; i < new_end; i += PGSIZE * PGSIZE)
    *(char **)i = i;

  for (i = prev_end + PGSIZE; i < new_end; i += PGSIZE * PGSIZE) {
    pid = fork();
    if (pid < 0) {
      printf("error forking\n");
      exit(1);
    } else if (pid == 0) {
      sbrk(-1L * REGION_SZ);
      *(char **)i = i;
      exit(0);
    } else {
      int status;
      wait(&status);
      if (status == 0) {
        printf("memory not unmapped\n");
        exit(1);
      }
    }
  }

  exit(0);
}

void loadfromdisktest(){
    char *m1, *m2;
    m1 =0;
    sbrk(PGSIZE*12);
    m2 = malloc(1);
    *(char**)m2 = m1;
    m1 = m2;
}

int
run(void f(char *), char *s) {
  int pid;
  int xstatus;
  
  printf("running test %s\n", s);
  if((pid = fork()) < 0) {
    printf("runtest: fork error\n");
    exit(1);
  }
  if(pid == 0) {
    f(s);
    exit(0);
  } else {
    wait(&xstatus);
    printf("xstatus: %d\n",xstatus);
    if(xstatus != 0 && xstatus != 1) 
      printf("test %s: FAILED\n", s);
    else
      printf("test %s: OK\n", s);
    return xstatus == 0 || xstatus == 1;
  }
}

int
main(int argc, char *argv[])
{
  char *n = 0;
  if(argc > 1) {
    n = argv[1];
  }
  
  struct test {
    void (*f)(char *);
    char *s;
  } tests[] = {
    { sparse_memory, "sparse_memory"},
    {sparse_memory_unmap, "sparse_memory_unmap"},
    {loadfromdisktest, "load from disk test"},
    { 0, 0},
  };
    
  printf("lazytests starting\n");

  int fail = 0;
  for (struct test *t = tests; t->s != 0; t++) {
    if((n == 0) || strcmp(t->s, n) == 0) {
      if(!run(t->f, t->s))
        fail = 1;
    }
  }
  if(!fail)
    printf("ALL TESTS PASSED\n");
  else
    printf("SOME TESTS FAILED\n");
  exit(1);   // not reached.
}