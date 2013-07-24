#include <stdio.h>
#include <time.h>

int main(void)
{
 clock_t t;
 unsigned long i;

 t = clock();
 for(i=0; i < 10000000; i++);
 printf("diff: %d\n",clock()-t);

 return 0;
};
