#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

static int char_to_int(const char *str);

int
main (int argc, char **argv)
{
  if(argc!=5){
      printf("argc : %d\n", argc);
      printf("argc must be 5.\n");
      return 0;
  }

  int array[5], i, a, b;
  for(i=1;i<5;i++){
    array[i] = char_to_int(argv[i]);
  }
  a = fibonacci(array[1]);
  b = max_of_four_int(array[1], array[2], array[3], array[4]);
  printf("%d %d\n", a, b);

  return EXIT_SUCCESS;
}

static int char_to_int(const char *str)
{
  int sign = 1;
  int result = 0;

  if (*str == '-') { sign = -1; str++; }
  else if (*str == '+') { str++; }

  for (; *str; str++) {
    if (*str < '0' || *str > '9') break;
    result = result * 10 + (*str - '0');
  }
  return sign * result;
}