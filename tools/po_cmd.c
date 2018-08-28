/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  char eval[10000];
  int i;
  int len = 0;
  for ( i = 1 ; i < argc ; i++ ) {
    if ( !strcmp(argv[i], "/in") ) {
      fgets(eval+len, sizeof(eval)-len-1, stdin);
    } else if ( !strcmp(argv[i], "/qin") ) {
      eval[len++] = '\"';
      fgets(eval+len, sizeof(eval)-len-1, stdin);
      len += strlen(eval+len);
      while ( eval[len-1] == '\n' || eval[len-1] == '\r' ) len--;
      eval[len++] = '\"';
    } else {
      strcpy(eval+len, argv[i]);
    }
    len += strlen(eval+len);
    eval[len] = ' ';
    len++;
  }

  system(eval);
  return 0;
}
