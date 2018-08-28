/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module inserts a file into another between two tags.
 * The tags are -GLOBAL-INSERT- and -GLOBAL_INSERT-END- inside a .c
 * style comment.
 * The insertion overwrites whatever existed betweem the 2 tags.
 *
 * This program is simple and does not handle pathetic cases.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Remove the \r added on Windows OS by fgets(). This flag is enabled
 * by the -nocr command line option.
 */
static int nocr = 0;

/* Maximum line length is 50000 characters
 */
#define MAX_LINE 50000
static char line[MAX_LINE];

/* My fgets which removes added \r if the command line option -nocr is
 * entered.
 */
static char *fgetsMine(char *line, int max, FILE *file)
{
  char *result = fgets(line, max, file);
  
  if ( nocr ) {
    // Get rid of '\r'
    int len = strlen(line);
    if ( line[len-2] == '\r' ) {
      line[len-2] = '\n';
      line[len-1] = 0;
    }
  }
  return result;
}

/* Insert file f1 inside f2 at the appropriate position, ie, between
 * GLOBAL-INSERT- labels. Erase whatever existed before between the two
 * labels. Output is written to fout. Can fout be equal to f2? This
 * may depend on the OS. Most likely it won't work.
 */
void insert(FILE *f1, FILE *f2, FILE *fout)
{
  int erase = 0;
  char *c;

  do {
    // Read one line from fout
    if ( fgetsMine(line, MAX_LINE, f2) == NULL ) return;

    // Write the line to fout unless it is supposed to be erased
    if ( !erase )
      fputs(line, fout);

    // Find "/" character
    for ( c = line ; *c != 0 ; c++ ) {
      if ( *c == '/' ) {
	// Check if we have any of the recognized symbols
	if ( (c[1] == '*' || c[1] == '/') ) {
	  if ( !strncmp(c+2, "-GLOBAL-INSERT-END-",
			sizeof("-GLOBAL-INSERT-END-")-1) ) {
	    erase = 0;
	    fputs(line, fout);
	  } else if ( !strncmp(c+2, "-GLOBAL-INSERT-",
			       sizeof("-GLOBAL-INSERT-")-1) ) {
	    // Copy paste f1 at this position in fout
	    fputc('\n', fout);
	    do {
	      if ( fgetsMine(line, MAX_LINE, f1) == NULL ) break;
	      fputs(line, fout);
	    } while ( 1 );
	    erase = 1;
	  }
	}
      }
    } // for loop
  } while ( 1 );
}

int main(int argc, char **argv)
{
  FILE *f1, *f2, *fout = stdout;
  char *name1 = NULL, *name2 = NULL;
  int i;
  int printError = 0;

  for ( i = 1 ; i < argc ; i++ ) {
    if ( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ) {
      printf("Usage: %s [-nocr] f1 f2 > output_file\n\n",
	     argv[0]);
      printf("  insert f1 inside f2. Write the output to output_file\n");
      printf("  -nocr: optional to remove \\r added by Windows OS.\n");
      exit(0);

    } else if ( !strcmp(argv[i], "-nocr") ) {
      nocr = 1;
    } else {
      if ( name1 == NULL ) name1 = argv[i];
      else if ( name2 == NULL ) name2 = argv[i];
      else
	printError = 1;
    }
  }

  if ( printError || name1 == NULL || name2 == NULL ) {
    printf("Incorrect parameters. Use --help for correct usage.\n");
    exit(0);
  }

  if ( (f1 = fopen(name1, "r")) == NULL ) {
    printf("Can't open file %s\n", name1);
    exit(0);
  }

  if ( (f2 = fopen(name2, "r")) == NULL ) {
    printf("Can't open file %s\n", name2);
    exit(0);
  }
  
  // Insert f1 in f2 and send output to fout
  insert(f1, f2, fout);

  fclose(f1);
  fclose(f2);

  return 0;
}
