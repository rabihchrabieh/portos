/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Removes C and C++ comments. Handles nested C comments (to be tested).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum line length
 */
#define MAX_LINE 100000
static char line[MAX_LINE];

// Nested C comment level
static int c_level = 0;

// C++ comment
static int cplusplus = 0;

/* Remove comments
 */
static void decomment(FILE *fin, FILE *fout)
{
  char *c;
  
  do {
    // Read one line from fin
    if ( fgets(line, MAX_LINE, fin) == NULL ) return;
    
    // Find "/" character
    for ( c = line ; *c != 0 ; c++ ) {
      if ( c[0] == '/' && c[1] == '*' && !cplusplus ) c_level++;
      else if ( c[0] == '/' && c[1] == '/' && c_level <= 0 ) cplusplus = 1;
      else if ( c_level > 0 && c[-1] == '/' && c[-2] == '*' ) c_level--;
      else if ( cplusplus && (c[0] == '\n' || c[0] == '\r') ) cplusplus = 0;
      
      if ( c_level <= 0 && cplusplus == 0 ) putc(*c, fout);
    }
  } while ( 1 );
}

/* main
 */
int main(int argc, char **argv)
{
  FILE *fin = stdin, *fout = stdout;
  int i;
  int printError = 0;

  for ( i = 1 ; i < argc ; i++ ) {
    if ( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ) {
      printf("Usage: %s < input_file > output_file\n\n",
	     argv[0]);
      printf("  input_file    a .c or .h file to decomment.\n");
      printf("  output_file   output is written to this file.\n");
      exit(0);

    } else {
      printError = 1;
    }
  }

  if ( printError ) {
    printf("Incorrect parameters. Use --help for correct usage.\n");
    exit(0);
  }

  decomment(fin, fout);

  return 0;
}
