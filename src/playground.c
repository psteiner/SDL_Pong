
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


void increment(int *p) {
  *p = *p + 1;
}

int main(void)
{
  int i = 10;
  int* j = &i;

  printf("foo!\n");

  printf("The value of i is %d\n", i);
  printf("The value of i is also %d\n", *j);
  increment(j);
  printf("i is now %d\n", i);

  return EXIT_SUCCESS;
}
