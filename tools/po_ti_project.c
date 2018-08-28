/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module reads a TI project, modifies it by adding necessary
 * per-file custom build, and creates the batch file for additional
 * compilation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *strsep(char **stringp, const char *delim);

/* Parse input.
 * Detect the strings at start of line:
 *   [Project Settings]
 *   CPUFamily
 *   Config
 *   [Source Files]
 *   Source
 *   ["Compiler" Settings: xxx]
 *   Options
 *
 * Within the Options line, detect
 *   $(yyy)
 * and change into
 *   %yyy%
 */

#define EXT_BAT ".po.bat"
#define EXT_TMP ".po.tmp"
#define EXT_OLD ".old"

char *BatchName;
char *TmpName;
char *OldName;
char *PortosPath;

char *CompilerPath;
char *CompilerName;

char *ProjName;
char *CPUFamily;

#define MAX_CONFIG 100
int nConfig = 0;
char *Config[MAX_CONFIG];
char *Options[MAX_CONFIG];

#define MAX_FILES 2000
int nFiles = 0;
char *Files[MAX_FILES];
char Custom[MAX_FILES][MAX_CONFIG] = {{0}};

int PortosCustom = 0;

// Get compiler name from family name
static char *getCompilerName(char *family)
{
  if ( !strncmp(family, "TMS320C2", strlen("TMS320C2")) )
    return "cl2000";
  else if ( !strncmp(family, "TMS320C54", strlen("TMS320C54")) )
    return "cl5000";
  else if ( !strncmp(family, "TMS320C55", strlen("TMS320C55")) )
    return "cl55";
  else if ( !strncmp(family, "TMS320C6", strlen("TMS320C6")) )
    return "cl6x";
  else if ( !strncmp(family, "TMS470R", strlen("TMS470R")) )
    return "cl470";
  else
    return NULL;
}

/* Warning pre-message.
 */
static void warning(void)
{
  //fprintf(stderr, "\" \", line 0: warning: ");
  fprintf(stderr, "  WARNING: ");
}

/* Failure pre-message.
 */
static void failure(void)
{
  //fprintf(stderr, "\" \", line 0: error: failed to convert TI project: ");
  fprintf(stderr, "  ERROR: failed to convert TI project: ");
}

/* Separate program and path.
 * prog is updated, and the path is returned.
 */
static void separatePath(char **prog, char **path)
{
  char *c = *prog + strlen(*prog) - 1;
  for ( ; c != *prog && *c != '/' && *c != '\\' ; c-- );
  if ( *prog != c ) {
    if ( path ) *path = *prog;
    *c = 0;
    *prog = c + 1;
  } else {
    if ( path ) *path = "";
  }
}

// Replace character in string
static void replaceChar(char *str, char findchar, char replacechar)
{
  char *c = str;
  while ( (c = strchr(c, findchar)) ) *c = replacechar;
}

/* Replaces a given string by another of equal size inside a longer string,
 * if it exists. Strings must be of equal size.
 */
static void replaceString(char *mainstr, char *findstr, char* replacestr)
{
  char *c;
  for ( c = mainstr ; *c ; c++ ) {
    if ( *c == *findstr ) {
      char *c1 = c+1, *d;
      for ( d = findstr+1 ; *d && *d == *c1 ; d++, c1++ );
      if ( *d == 0 ) {
	// Found string. Replace it with new one
	for ( d = replacestr ; *d ; d++, c++ ) *c = *d;
      }
    }
  }
}

/* Locates a string inside an array of strings, if present.
 * A zero or positive index is returned if the string is found, -1 otherwise.
 */
int stringIndex(char *array[], int size, char *string)
{
  int i;
  for ( i = 0 ; i < size ; i++ ) {
    if ( !strcmp(array[i], string) ) return i;
  }
  return -1;
}

/* Allocates memory, copies the string and returns a pointer to it.
 * If malloc fails, the program halts.
 */
char *copyString(char *string)
{
  char *s = malloc(strlen(string)+1);
  if ( !s ) {
    failure();
    fprintf(stderr, "memory allocation failure.\n");
    exit(1);
  }
  strcpy(s, string);
  return s;
}

/* Parse input and extract info.
 */
static void parse(FILE *fin)
{
  enum {
    NONE,
    PROJECT_SETTINGS,
    SOURCE_FILES,
    COMPILER_SETTINGS,
    FILE_SETTINGS,
    OTHER
  } state = NONE;

  char line[10000];
  int configIndex = 0;
  char *token;

  while ( fgets(line, sizeof(line), fin) ) {

    char *line0 = line;
    if ( *line0 == ' ' || *line0 == '\t' ) strsep(&line0, " \t");

    if ( *line0 == '[' ) {
      line0++;
      token = strsep(&line0, "]");

      if ( !strcmp(token, "Project Settings") )
	state = PROJECT_SETTINGS;
      else if ( !strcmp(token, "Source Files") )
	state = SOURCE_FILES;
      else if ( !strncmp(token, "\"Compiler\" Settings",
			 sizeof("\"Compiler\" Settings") - 1) ) {
	char *t;
	token += sizeof("\"Compiler\" Settings");
	t = strsep(&token, "\"");
	t = strsep(&token, "\"");
	state = COMPILER_SETTINGS;
	// Locate config index
	configIndex = stringIndex(Config, nConfig, t);
	if ( configIndex < 0 ) {
	  failure();
	  fprintf(stderr, "unknown config \"%s\".\n", t);
	  exit(1);
	}
      } else {
	char *t;
	int index, config;
	state = OTHER;
	t = strsep(&token, "\"");
	if ( token ) t = strsep(&token, "\"");
	index = stringIndex(Files, nFiles, t);
	if ( index >= 0 ) {
	  state = FILE_SETTINGS;
	  t = strsep(&token, "\"");
	  if ( token ) t = strsep(&token, "\"");
	  config = stringIndex(Config, nConfig, t);
	  if ( config >= 0 ) Custom[index][config] = 1;
	  else {
	    failure();
	    fprintf(stderr, "unknown config \"%s\".\n", t);
	    exit(1);
	  }
	}
      }
    } else if ( state == PROJECT_SETTINGS &&
		!strncmp(line0, "CPUFamily", sizeof("CPUFamily")-1) ) {
      line0 += sizeof("CPUFamily")-1;
      token = strsep(&line0, " \t=");
      token = strsep(&line0, " \t\r\n");
      CPUFamily = copyString(token);
    } else if ( state == PROJECT_SETTINGS &&
		!strncmp(line0, "Config", sizeof("Config")-1) ) {
      line0 += sizeof("Config")-1;
      token = strsep(&line0, "\"");
      token = strsep(&line0, "\"");
      Config[nConfig] = copyString(token);
      if ( ++nConfig >= MAX_CONFIG ) {
	failure();
	fprintf(stderr, "exceeded max number of configurations (%d).\n",
		MAX_CONFIG);
	exit(1);
      }
    } else if ( state == SOURCE_FILES &&
		!strncmp(line0, "Source", sizeof("Source")-1) ) {
      int l;
      line0 += sizeof("Source")-1;
      token = strsep(&line0, "\"");
      token = strsep(&line0, "\"");
      l = strlen(token);
      if ( !strcmp(token+l-2, ".c") ||
	   !strcmp(token+l-4, ".cpp") ) {
	Files[nFiles] = copyString(token);
	if ( ++nFiles >= MAX_FILES ) {
	  failure();
	  fprintf(stderr, "exceeded max number of files (%d).\n", MAX_FILES);
	  exit(1);
	}
      }
    } else if ( state == COMPILER_SETTINGS &&
		!strncmp(line0, "Options", sizeof("Options")-1) ) {
      line0 += sizeof("Options")-1;
      token = strsep(&line0, " \t=");
      token = strsep(&line0, "\r\n");
      Options[configIndex] = copyString(token);
      replaceString(Options[configIndex], "$(Proj_dir)", "%Proj_dir_%");
    } else if ( state == FILE_SETTINGS ) {
      if ( strstr(line0, "CustomBuildCmd") && strstr(line0, ".po.bat") &&
	   strstr(line0, "$(File_basename)") ) {
	PortosCustom = 1;
      }
    }
  }
}

/* Create output files
 */
static void emit(FILE *fout, FILE *fpo)
{
  int i, j;
  int first = 1;

  for ( i = 0 ; i < nFiles ; i++ ) {
    for ( j = 0 ; j < nConfig ; j++ ) {
      if ( !Custom[i][j] ) {
	if ( first ) {
	  first = 0;
	  if ( !PortosCustom ) {
	    char str[1000];
	    sprintf(str, "copy /Y %s %s", ProjName, OldName);
	    system(str);
	  }
	}
	fprintf(fout, "[\"%s\" Settings: \"%s\"]\n", Files[i], Config[j]);
	fprintf(fout, "UseCustomBuild=true\n");
	fprintf(fout, "CustomBuildCmd=\"$(Proj_dir)\\%s.po.bat\" \"$(Proj_dir)\" \"$(Proj_cfg)\" $(File_basename) $(File_name)\n", ProjName);
	fprintf(fout, "CustomBuildOutput=\"$(Proj_dir)\\$(Proj_cfg)\\$(File_basename).pp\"\n");
	fprintf(fout, "CustomBuildOutput=\"$(Proj_dir)\\$(Proj_cfg)\\$(File_basename).po\"\n");
	fprintf(fout, "CustomBuildOutput=\"$(Proj_dir)\\$(Proj_cfg)\\$(File_basename).obj\"\n\n");
      } else if ( !PortosCustom ) {
	warning();
	fprintf(stderr, "custom instructions are used by file %s, config \"%s\". Automatic conversion will not be performed for this file. Manual handling is required.\n", Files[i], Config[j]);
      }
    }
  }
  
  if ( first == 0 ) {
    fprintf(stdout, "  Modified project file\n");
    warning();
    fprintf(stderr, "you may have to reload the project and rebuild any files newly added\n");
  }

  // Obtain compiler path from registry
  if ( 1 ) {
    char str[1000];
    FILE *fp;

    sprintf(str, "reg query \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Texas Instruments\" /s > %s", TmpName);

    system(str);
    
    fp = fopen(TmpName, "r");
    if ( !fp ) {
      warning();
      fprintf(stderr, "file read/write error. Could not access registry to determine compiler path. You have to manually set Compiler_path in batch file %s.\n", BatchName);
      CompilerPath = "";

    } else {
      char line[5000];
      CompilerPath = NULL;
      char *avail;
      do {
	avail = fgets(line, sizeof(line), fp);

	if ( avail && strstr(line, "CCS_") && strstr(line, CPUFamily) &&
	     strstr(line, "Build Tools") ) {

	  if ( (avail = fgets(line, sizeof(line), fp)) ) {
	    CompilerPath = strstr(line, "CGT_");
	    if ( CompilerPath ) break;
	  }
	}
      } while ( avail );
	
      fclose(fp);
      sprintf(str, "del \"%s\"", TmpName);
      system(str);

      if ( !CompilerPath ) {
	warning();
	fprintf(stderr, "could not parse registry to determine compiler path. You have to manually set Compiler_path in batch file %s.\n", BatchName);
	CompilerPath = "";
      } else {
	char *c;
	CompilerPath += 4;
	// Convert pipe to backslash
	replaceChar(CompilerPath, '|', '\\');
	// Remove \n and \r
	c = CompilerPath;
	if ( (c = strchr(CompilerPath, '\n')) ) *c = 0;
	if ( c[-1] == '\r' ) c[-1] = 0;
      }
    }
  }

  // Find compiler name
  CompilerName = getCompilerName(CPUFamily);
  if ( !CompilerName ) {
    warning();
    fprintf(stderr, "cannot find compiler name based on CPUFamily. You have to manually set Compiler in batch file %s.\n", BatchName);
    CompilerName = "";
  }

  // Create .po.bat file
  fprintf(fpo, "@echo off\n\n");
  fprintf(fpo, "REM Portos auto-generated batch file\n\n");
  fprintf(fpo, "set Compiler_path=%s\n", CompilerPath);
  fprintf(fpo, "set Compiler=\"%%Compiler_path%%\\%s\"\n", CompilerName);
  fprintf(fpo, "set Proj_dir_=%%1\n");
  fprintf(fpo, "set Config=%%2\n\n");
  
  for ( j = 0 ; j < nConfig ; j++ )
    fprintf(fpo, "if %%Config%% == \"%s\" set Options=%s\n\n", Config[j], Options[j]);

  fprintf(fpo, "if exist \"%%Proj_dir_%%\\%%Config%%\\%%3.pp\" del \"%%Proj_dir_%%\\%%Config%%\\%%3.pp\"\n");
  fprintf(fpo, "if exist \"%%Proj_dir_%%\\%%Config%%\\%%3.po\" del \"%%Proj_dir_%%\\%%Config%%\\%%3.po\"\n\n");

  fprintf(fpo, "@echo [C Preprocessing %%4]\n");
  fprintf(fpo, "@echo on\n");
  fprintf(fpo, "%%Compiler%% -ppl %%Options%% %%4\n");
  fprintf(fpo, "@echo off\n\n");
  fprintf(fpo, "if not exist %%3.pp goto :terminate\n\n");

  fprintf(fpo, "@echo [Portos Preprocessing %%3.pp]\n");
  if ( *PortosPath )
    fprintf(fpo, "\"%s\\po_preprocess\" %%3.pp %%3.po\n\n", PortosPath);
  else
    fprintf(fpo, "po_preprocess\" %%3.pp %%3.po\n\n");
  fprintf(fpo, "if not exist %%3.po goto :terminate\n\n");

  fprintf(fpo, "@echo [Compiling %%3.po]\n");
  fprintf(fpo, "@echo on\n");
  fprintf(fpo, "%%Compiler%% %%Options%% %%3.po\n");
  fprintf(fpo, "@echo off\n\n");

  fprintf(fpo, ":terminate\n");

  fprintf(fpo, "if exist %%3.pp move %%3.pp \"%%Proj_dir_%%\\%%Config%%\\.\"\n");
  fprintf(fpo, "if exist %%3.po move %%3.po \"%%Proj_dir_%%\\%%Config%%\\.\"\n\n");

  fprintf(fpo, "REM Call user-defined program and arguments if any\n");
  fprintf(fpo, "%%5 %%6 %%7 %%8 %%9\n\n");

  fprintf(stdout, "  Created batch file %s\n", BatchName);
}

/* Compares the dates of two files
 */
time_t datecmp(const char *f1, const char *f2)
{
  struct stat st;
  time_t t1, t2;
  stat(f1, &st);
  t1 = st.st_mtime;
  if ( stat(f2, &st) < 0 ) t2 = 0;
  else t2 = st.st_mtime;
  return t1 - t2;
}

/* Main function
 */
static void process(char *name)
{
  FILE *fin, *fpo;

  // Convert slashes to backslashes
  replaceChar(name, '/', '\\');

  // Project file name
  ProjName = copyString(name);
  separatePath(&ProjName, NULL);

  // Batch file name
  BatchName = malloc(strlen(name) + sizeof(EXT_BAT) + 1);
  strcpy(BatchName, name);
  strcat(BatchName, EXT_BAT);

  // Tmp name
  TmpName = malloc(strlen(name) + sizeof(EXT_TMP) + 1);
  strcpy(TmpName, name);
  strcat(TmpName, EXT_TMP);

  // Old name (if needed)
  OldName = malloc(strlen(name) + sizeof(EXT_TMP) + 1);
  strcpy(OldName, name);
  strcat(OldName, EXT_OLD);

  fin = fopen(name, "r+");
  if ( !fin ) {
    failure();
    fprintf(stderr, "failed to open project file %s.\n", name);
    exit(1);
  }

  if ( datecmp(name, BatchName) < 0 ) {
    // Batch is newer, nothing to be done
    fprintf(stdout, "  Project file is up-to-date\n");
    return;
  }

  fpo = fopen(BatchName, "w");
  if ( !fpo ) {
    failure();
    fprintf(stderr, "failed to create batch file %s.\n", BatchName);
    exit(1);
  }  

  parse(fin);
  emit(fin, fpo);
  fclose(fin);
}

/* main
 */
int main(int argc, char **argv)
{
  char *prog = copyString(argv[0]);

  if ( argc < 2 ) {
    fprintf(stderr, "Usage: %s project_file.\n", argv[0]);
    return 0;
  }

  fprintf(stdout, "  Remember to save project file if you make modifications\n");

  separatePath(&prog, &PortosPath);

  process(argv[1]);
  return 0;
}
