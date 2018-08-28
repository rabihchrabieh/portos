/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module extracts global prototypes from .c or .h files.
 * It scans for -GLOBAL- strings in the input file (.c or .h).
 * It extracts the function prototype and write it to the output.
 * The output could be inserted in a .h file or a .doc file.
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

/* Extract -GLOBAL-. It returns a pointer to the current
 * position in the file (current position in the line).
 * The terminating character after a -GLOBAL- is found is either a "{"
 * or a ";" (to extract from .h files, the ";" is useful).
 */
static char *extractGlobal(FILE *fin, FILE *fout, char *line)
{
  int comment = 1;

  do {
    char *c;
    // Search line for terminating char outside a comment
    for ( c = line ; *c != 0 ; c++ ) {
      if ( c[0] == '/' ) {
	if ( c[1] == '*' ) comment = 1;
	else if ( c[1] == '/' ) {
	  // This is a comment line. Ignore rest of it.
	  comment = 0;
	  break; // from for loop
	}
      }
      if ( c[0] == '*' && c[1] == '/' ) comment = 0;

      // If we're not inside a comment, look for terminating char
      if ( comment == 0 && (c[0] == '{' || c[0] == ';') ) {
	// Done. Print up to char c-1 and stop
	char c_save = *c;
	*c = 0;
	fputs(line, fout);
	*c = c_save;
	return c;
      }
    } // for loop

    // No terminating char. Print this line
    fputs(line, fout);

    // Read next line
    if ( fgetsMine(line, MAX_LINE, fin) == NULL ) return NULL;
  } while ( 1 );
}

/* Extract GLOBALs from file fin and send to file fout
 */
static void extract(FILE *fin, FILE *fout)
{
  char *c;
  
  do {
    // Read one line from fin
    if ( fgetsMine(line, MAX_LINE, fin) == NULL ) return;
    
    // Find "/" character
    for ( c = line ; *c != 0 ; c++ ) {
      if ( *c == '/' ) {
	// Check if we have any of the recognized symbols
	if ( (c[1] == '*' || c[1] == '/') ) {
	  if ( !strncmp(c+2, "-GLOBAL-", sizeof("-GLOBAL-")-1) ) {
	    // Ignore a -GLOBAL-INSERT-
	    if ( strncmp(c+2, "-GLOBAL-INSERT-",
			 sizeof("-GLOBAL-INSERT-")-1) ) {
	      c = extractGlobal(fin, fout, c);
	      fprintf(fout, ";\n\n");
	      if ( c == NULL ) return;
	    }
	  }
	}
      }
    } // for loop
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
      printf("Usage: %s [-nocr] < input_file > output_file\n\n",
	     argv[0]);
      printf("  input_file    a .c or .h file to extract GLOBALs from.\n");
      printf("  output_file   output is written to this file.\n");
      printf("  -nocr         optional to remove \\r added by Windows OS.\n");
      exit(0);

    } else if ( !strcmp(argv[i], "-nocr") ) {
      nocr = 1;
    } else {
      printError = 1;
    }
  }

  if ( printError ) {
    printf("Incorrect parameters. Use --help for correct usage.\n");
    exit(0);
  }

  // Extract GLOBALs to a tmp file
  extract(fin, fout);

  return 0;
}
