/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module replaces one (or more) string(s) in a file by some
 * other string(s). A command file is used that lists for each
 * string to be replaced what is the corresponding new string.
 *
 * Command file: "first" character must be the delimiter. Then follow
 * pairs of strings, first string is the one to be replaced, second string
 * is the replacement. Each string must be delimited by the delimiter
 * character on both side, at the exact locations. The delimiter character
 * cannot exist inside the strings.
 * E.g. of command file:
 * $
 * $abc$     $def$
 * $qwerty$  $azerty$
 * This command file replaces all "abc" with "def", and all "qwerty" with
 * "azerty" (in the input files).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pool that holds commands and files. Increase size if not enough.
 * The start of pool will contain the command file. Afterwards come
 * the command instructions (decoded from the file). Then follows the
 * input file. Then follows the output file.
 */
enum { replace_MAX = 1000000};

/* One string pair replacement intstruction is stored in this database
 */
typedef struct {
  char *old;  // old string
  char *new;  // new string
} Pair;

/* The list of commands (list of old and new strings). This list will
 * be inserted in pool after the command file. Make sure it is properly
 * aligned.
 */
typedef struct {
  int count;   // Number of commands

  // Keep this last. Length will increase.
  Pair pair[0];
} List;

/* Data
 */
static struct {
  char pool[replace_MAX];
  char delimiter;
  char *command;    // Command file (containing strings and replacements)
  List *list;       // List of instructions
  char *file;       // Input file
  char *fileout;    // Output file
} Data;

/* Read command file. First character must be the delimiter. Then follow
 * pairs of strings, first string is the one to be replaced, second string
 * is the replacement. Each string must be delimited by the delimiter
 * character on both side, at the exact locations.
 * E.g. of command file:
 * $
 * $abc$     $def$
 * $qwerty$  $azerty$
 * This file replaces all "abc" with "def", and all "qwerty" with
 * "azerty".
 */
static void replace_command(char *filename)
{
  FILE *f;
  int size;
  char *c = Data.command = Data.pool;
  char *stringStart = NULL;
  int parity = 0;
  long long listLocation;
  int i;

  if ( (f = fopen(filename, "r")) == NULL ) {
    printf("ERROR: can't open command file %s\n", filename);
    exit(0);
  }

  // Read command file at start of pool
  size = fread(Data.command, 1, replace_MAX, f);
  fclose(f);
  if ( size >= replace_MAX ) {
    printf("ERROR: command file %s too large\n", filename);
    exit(0);
  }

  // Set delimiter
  Data.delimiter = c[0];

  // Locate the list of instructions after this file. Round up alignment
  // to a multiple of 8.
  listLocation = (long long)Data.pool + size;
  listLocation = ((listLocation+7) >> 3) << 3;
  Data.list = (void*)listLocation;
  Data.list->count = 0;

  // Search for pairs of strings
  for ( i = 1 ; i < size ; i++ ) {
    if ( c[i] == Data.delimiter ) {
      if ( stringStart ) {
	// This is the end of a string
	if ( parity == 0 ) {
	  // This is the first in a pair
	  int p = Data.list->count;
	  Data.list->pair[p].old = stringStart;
	  // Look for co-string
	  parity = 1;

	} else {
	  // This is the second in a pair, ie, co-string or replacement
	  // string
	  int p = Data.list->count++;
	  Data.list->pair[p].new = stringStart;
	  // Look for new pair
	  parity = 0;
	}

	// Look for new string
	stringStart = NULL;

      } else {
	// This is the start of a string
	stringStart = c + i + 1;
      }
    }
  }

  // Check that we got pairs of strings
  if ( parity == 1 ) {
    printf("ERROR: missing replacement string in command file %s\n",
	   filename);
    exit(0);
  }

  // Check if we got at least one pair of strings
  if ( Data.list->count <= 0 ) {
    printf("Empty command file. Nothing to be done\n");
    exit(0);
  }

  // Locate the position of the input files inside pool. This is right
  // after the instructions
  Data.file = (char*)Data.list + sizeof(List) + Data.list->count*sizeof(Pair);
}

/* Replace strings in file
 */
void replace_replace(char *filename)
{
  FILE *f;
  int size;
  char *c = Data.file;
  int i;
  char *o;
  int modified = 0;

  if ( (f = fopen(filename, "r")) == NULL ) {
    printf("WARNING: can't open file %s. Ignoring this file\n", filename);
    return;
  }

  // Read file
  size = fread(Data.file, 1, replace_MAX - (c-Data.pool), f);
  fclose(f);
  if ( size >= replace_MAX - (c-Data.pool) ) {
    printf("WARNING: file %s too large. Ignoring it\n", filename);
    return;
  }

  // Insert one delimiter character after the input file to ensure
  // loops stop there
  Data.file[size] = Data.delimiter;
  // Locate the output file inside pool
  o = Data.fileout = Data.file + size + 1;

  // For each position in file
  for ( i = 0 ; i < size ; i++ ) {
    // Look if it matches a string in the instructions list. A more powerful
    // search can be implemented with a large hash table. Each key in the
    // hash table is formed with N characters. But we don't care for speed
    // here.
    int j;

    // First copy character to ouput file. Undo later if necessary.
    *o++ = c[i];
    if ( o - Data.pool >= replace_MAX ) {
      printf("ERROR: file %s too large. Ignoring\n", filename);
      return;
    }

    for ( j = 0 ; j < Data.list->count ; j++ ) {
      if ( c[i] == Data.list->pair[j].old[0] ) {
	// First character agrees, check the remaining ones
	char *d = Data.list->pair[j].old + 1;
	char *e = c + i + 1;
	for ( ; *d != Data.delimiter && *d == *e ; d++, e++ );
	if ( *d == Data.delimiter ) {
	  // Matching string
	  // Set the modified flag to 1
	  modified = 1;
	  // Update index i (skip characters that are part of a matching
	  // string).
	  i = e - Data.file - 1;
	  // Replace with new string.
	  for ( o--, e = Data.list->pair[j].new ; *e != Data.delimiter ; ) {
	    *o++ = *e++;
	    // NOTE: should move (and modify) this test outside the loop
	    if ( o - Data.pool >= replace_MAX ) {
	      printf("ERROR: file %s too large. Ignoring\n", filename);
	      return;
	    }
	  }
	  // Stop search over other strings for this character
	  break;
	}
      }	
    }
  }

  if ( modified ) {
    // Write to file
    if ( (f = fopen(filename, "w")) == NULL ) {
      printf("WARNING: can't update file %s. Ignoring this file\n",
	     filename);
      return;
    }
    printf("%s\n", filename);
    size = fwrite(Data.fileout, 1, o - Data.fileout, f);
    fclose(f);
    if ( size < o - Data.fileout ) {
      printf("WARNING: failed to write entire file %s\n", filename);
      return;
    }
  }
}

/* Main
 */
int main(int argc, char **argv)
{
  char *namecmd = NULL, *name;
  int i;

  for ( i = 1 ; i < argc ; i++ ) {
    if ( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ) {
      printf("Usage: %s fcmd f1 [f2 f3 ... ]\n\n",
	     argv[0]);
      printf("  fcmd is a file containing the instructions\n");
      printf("  f1, f2, ...: files in which strings are to be replaced\n");
      printf("  First character in fcmd is the delimiter character, eg, $\n"
	     "  then follows pairs of strings, each string delimited, on\n"
	     "  both sides, by the delimiter character. White spaces can\n"
	     "  exist between strings, outside the delimited areas\n\n");
      exit(0);

    } else if ( namecmd == NULL ) {
	namecmd = argv[i];
	replace_command(namecmd);

    } else {
      name = argv[i];
      replace_replace(name);
    }
  }

  return 0;
}
