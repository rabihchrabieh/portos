/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Removes debug flags to produce a debug only version of the source file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum line length
 */
#define MAX_LINE 100000
static char line[MAX_LINE];

/* Finds a string inside a string.
 * It returns the position of s2 inside s1, and -1 if s2 is not found in s1.
 */
static int strfind(const char *s1, const char *s2)
{
  const char *s, *c1, *c2;
  for ( s = s1 ; *s ; s++ ) {
    for ( c1 = s, c2 = s2 ; *c1 == *c2 && *c1 && *c2 ; c1++, c2++ );
    if ( !*c2 ) return s - s1;
  }
  return -1;
}

/* Remove debug flags
 */
static void debugonly(FILE *fin, FILE *fout)
{
  int ignore = 0;
  int ignoreNext = 0;

  do {

    // Read one line from fin
    if ( fgets(line, MAX_LINE, fin) == NULL ) return;

    if ( strfind(line, "DEBUG_MODE") >= 0 ) {
      if ( strfind(line, "NON_DEBUG_MODE_START") >= 0 ) {
	ignore = 1;
	ignoreNext = 1;
      } else if ( ignoreNext == 1 &&
		  strfind(line, "NON_DEBUG_MODE_END") >= 0 ) {
	ignoreNext = 0;
      } else {
	// DEBUG_MODE: delete this line
	ignore = 1;
      }
    }
    if ( !ignore ) fputs(line, fout);
    ignore = ignoreNext;

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

  debugonly(fin, fout);

  return 0;
}
