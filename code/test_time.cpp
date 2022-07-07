#include "stdio.h"
#include "windows.h"

typedef unsigned __int64 u64;

int main(){
  u64 before  = __rdtsc();

  int count = 0;
  for(int i = 0; i < 1; i++){
    count++;
  }

  u64 after = __rdtsc();
  printf_s("%I64d ticks diff\n", after - before);
}

