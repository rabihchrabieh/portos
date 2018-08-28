/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Looks for a string in binary files and displays a few characters before
 * and after.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* System failure reporting
 */
static void failure(char *msg)
{
  fprintf(stderr, "  mygrep: %s\n", msg);
  exit(1);
}

/* Display string containing zeros
 */
static void myprint(char *str, int len)
{
  int i;
  for ( i = 0 ; i < len ; i++ ) {
    if ( isprint((int)str[i]) ) printf("%c", str[i]);
    else printf(".");
  }
}

/* Main thing
 */
static void mainThing(char *str, char *input, int disp_len, int input_len,
		      char *inputName)
{
  char *s;
  int i;
  int first_time = 1;
  int str_len = strlen(str);

  for ( s = input, i = 0 ; i < input_len ; s++, i++ ) {
    if ( *s == *str ) {
      char save = s[str_len];
      s[str_len] = 0;
      if ( !strcmp(s+1, str+1) ) {
	// Found string
	char *before = s-disp_len;
	if ( before - input < 0 ) before = input;
	s[str_len] = save;
	if ( first_time && inputName ) {
	  printf("In %s\n", inputName);
	  first_time = 0;
	}
	myprint(before, s-before+str_len+disp_len);
	printf("\n");
      } else {
	s[str_len] = save;
      }
    }
  }
}

/* Main loop
 */
static void mainLoop(char *str, char *inputName, int disp_len, int disp_file)
{
  int input_len;
  char *input;
  FILE *f = fopen(inputName, "r");
  if ( !f ) failure("can't open input file");

  fseek(f, 0, SEEK_END);
  input_len = ftell(f);
  fseek(f, 0, SEEK_SET);
  input = (char*)calloc(input_len+1+500, 1);
  if ( !input ) failure("can't allocate memory for input file");

  fread(input, 1, input_len, f);
  fclose(f);
  
  input[input_len] = 0; // Maybe needed for some comparisons

  mainThing(str, input, disp_len, input_len, disp_file ? inputName : NULL);

  free(input);
}

static void usageHelp(void)
{
  printf("  usage: mygrep [Options] string input_file\n");
  printf("           Finds string inside binary input_file\n");
  printf("           Options:\n");
  printf("             -cn\tdisplay n characters before and after string\n");
  printf("             -f\tdisplay file name\n");
  printf("            --help\tthis help message\n");
}

/* main
 */
int main(int argc, char **argv)
{
  int i;
  int disp_len = 30;
  int disp_file = 0;
  char *str;

  for ( i = 1 ; i < argc-2 ; i++ ) {
    if ( argv[i][0] != '-' ) break;

    if ( argv[i][1] == 'c' ) {
      disp_len = atoi(argv[i]+2);
    } else if ( !strcmp(argv[i], "-f" ) ) {
      disp_file = 1;
    } else if ( !strcmp(argv[i], "--help") ) {
      usageHelp();
      exit(1);
    } else {
      char msg[1000];
      sprintf(msg, "unrecognized option %s", argv[i]);
      failure(msg);
    }
  }

  str = argv[i];
  
  for ( ++i ; i < argc ; i++ ) {
    mainLoop(str, argv[i], disp_len, disp_file);
  }
  return 0;
}
