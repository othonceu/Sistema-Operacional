#include "types.h"
#include "stat.h"
#include "user.h"

int
power10(int n) {
  int val = 1;
  while (n--) {
    val *= 10;
  }
  return val;
}

int
char_to_int(char c) {
  return c - '0';
}

int
str_to_int(char *str) {
  int len, val = 0, d, i;

  for (len = 0; str[len]; len++);

  for (i = len - 1; i >= 0; i--) {
    d = char_to_int(str[i]);
    val += d * power10(len - 1 - i);
  }

  return val;
}

int
main(int argc, char *argv[])
{
  int i;
  for (i = 1; i < argc; i++) {
    printf(1, "%s = %d\n", argv[i], str_to_int(argv[i]));
    int tickets = str_to_int(argv[i]);

    if (fork(tickets) == 0) {
      for(;;);
    }
  }

  for (i = 1; i < argc; i++) {
    wait();
  }
  exit();
}
