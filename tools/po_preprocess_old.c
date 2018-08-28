/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Preprocesses C files to convert real time instructions into
 * standard C instructions. This utility works for most common
 * cases but may not handle complicated function declarations.
 * It handles the following directives:
 *   po_priority(P,T)
 *   po_signal(S,G,H), po_signalp   for overloading
 *   po_time(T,C,H), po_timep       for overloading
 *
 * The utility is called with 2 arguments, an input and an output
 * file names. It works after the C preprocessor.
 */

//-----------------------------------------------------------------
// Example

#if 0
void func(po_priority(P), int a, float b[10], struct mystruct c, double *d,
	  void (*e)(int x, float y))
{
  // Content
}
#endif

//-----------------------------------------------------------------
// Example converted

#if 0
/* Prototype */
void func(po_priority(P), int a, float b[10],
	  struct mystruct c, double *d,
	  void (*e)(int x, float y));

/* Old entry */
static void func_po_original(po_priority, int a, float b[10],
	  struct mystruct c, double *d,
	  void (*e)(int x, float y));

/* Priority function handle. Includes function arguments */
typedef struct {
  po_function_Handle pfhandle; /* MUST BE FIRST */
  struct {
    int a;
    void *b;
    struct mystruct c;
    double *d;
    void *e;
  } args;
} func_po_Handle;

/* Entry point from scheduler */
static void func_po_entry_scheduler(po_function_Handle *pfhandle)
{
  func_po_Handle *handle = (func_po_Handle*)pfhandle;
  func_po_original((po_function_ServiceHandle*)0, handle->args.a, handle->args.b, handle->args.c, handle->args.d, handle->args.e);
  po_free(handle);
}

/* New entry point */
void func(po_priority(P), int a, float b[10], struct mystruct c, double *d,
	  void (*e)(int x, float y))
{
  int po__priority = (P);
  func_po_Handle *po__pfhandle;
  #if !po_function_TRACK_NAME
  void *po__funcname = (void*)func;
  #else
  void *po__funcname = (void*)"func";
  #endif

  if ( !po__srvhandle && po__priority > po_function_getpri() ) {
    /* Immediate call */
    po_function_enternow(po__priority, po__funcname);
    func_po_original((po_function_ServiceHandle*)0, a, b, c, d, e);
    po_function_exitnow();
    return;
  }

  /* Not an immediate call, pack arguments inside handle */
  po__pfhandle = po_smalloc_const(sizeof(*po__pfhandle));
  if ( !po__pfhandle ) po_memory_error();

  po__pfhandle->args.a = a;
  po__pfhandle->args.b = b;
  po__pfhandle->args.c = c;
  po__pfhandle->args.d = d;
  po__pfhandle->args.e = e;

  #if po_function_TRACK_NAME
  po__pfhandle->pfhandle.name = po__funcname;
  #endif

  po_function_service((po_function_Handle*)po__pfhandle, po__srvhandle, func_po_entry_scheduler, po__priority);
}

/* Old entry */
static void func_po_original(po_priority(P), int a, float b[10], struct mystruct c, double *d,
	  void (*e)(int a, float b))
{
  // Content
}
#endif

//-----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Is it a TI platform?
 */
static int TI = 0;

/* Input output files
 */
static char FileName[1000];
static char FinName[1000];
static char *FoutName;
static FILE *Fin;
static FILE *Fout;

/* Input file is entirely buffered to facilitate string manipulation
 */
static char *Input;
static int InputLen;

// Current line number
static int FinLine = 1; // Original file line numbering
static int Line = 1;    // Relative to #line commands
static int Line0 = 1;   // Temporary

/* State variables
 */
#define N_LEVELS   1000
static char State[N_LEVELS];
static int Phrase[N_LEVELS];
static int Symbol[N_LEVELS];
static int Substate = 0, Substate1 = 0, Symstate = 0;
static int SymbolStart = 0;
static int SymbolStartStrobe = 0, SymbolEndStrobe = 0;
static int Level = 0;
static int CR = 0;
static int LineNumbering = 1;
static int FoundPF = 0; // Currently unused

// One priority function description
#define N_ARGS     100
#define N_CONTEXTS 1  // No more multiple contexts
struct {
  int lStart;
  int lEnd;
  int pDeclStart;
  int pDeclEnd;
  int pFuncNameStart;
  int pFuncNameEnd;
  int pPriorityKeyword;
  int pPriorityStart;
  int pPriorityEnd;
  int lPriority;
  int nargs;          // Excluding priority argument
  struct {
    int useVoidPointer;
    int pDeclStart;
    int pDeclEnd;
    int pNameStart;
    int pNameEnd;
    int lStart;
  } a[N_ARGS];
} PF_Vec[N_CONTEXTS], *PF = PF_Vec;

/* System message
 */
static void message(char *msg)
{
  fprintf(stdout, "%s\n", msg);
}

/* System failure reporting
 */
static void failure(char *msg)
{
  if ( FinName[0] != 0 ){
    if ( FinLine == Line && !strcmp(FinName, FileName) ) {
      if ( !TI ) fprintf(stderr, "  %s:%d: failure: ", FinName, FinLine);
      else fprintf(stderr, "\"%s\", line %d: failure: ", FinName, FinLine);
    } else {
      if ( !TI ) fprintf(stderr, "  %s:%d (%s:%d): failure: ",
			 FileName, Line, FinName, FinLine);
      else fprintf(stderr, "\"%s\", line %d (%s:%d): failure: ",
		   FileName, Line, FinName, FinLine);
    }
  } else {
    fprintf(stderr, "  failure: ");
  }
  fprintf(stderr, msg);
  fprintf(stderr, "\n");

  // Wipe content of output file
  if ( Fout ) {
    fclose(Fout);
    fopen(FoutName, "w");
  }
  exit(1);
}

/* Error reporting
 */
static void error(char *msg, int n)
{
  if ( FinName[0] != 0 ) {
    if ( FinLine == Line && !strcmp(FinName, FileName) ) {
      if ( !TI ) fprintf(stderr, "  %s:%d: error: ", FinName, FinLine);
      else fprintf(stderr, "\"%s\", line %d: error: ", FinName, FinLine);
    } else {
      if ( !TI ) fprintf(stderr, "  %s:%d (%s:%d): error: ",
			 FileName, Line, FinName, FinLine);
      else fprintf(stderr, "\"%s\", line %d (%s:%d): error: ",
		   FileName, Line, FinName, FinLine);
    }
  } else {
    fprintf(stderr, "  error: ");
  }
  fprintf(stderr, msg, n);
  fprintf(stderr, "\n");

  // Wipe content of output file
  if ( Fout ) {
    fclose(Fout);
    fopen(FoutName, "w");
  }
  exit(1);
}

/* Writes output file from position1 to position2-1 in Input
 */
static void output(int position1, int position2)
{
  fwrite(Input+position1, 1, position2-position1, Fout);
}

/* Output a string corresponding to type c
 */
static void outputdef(char c)
{
  switch ( c ) {
  case 'D':
    output(PF->pDeclStart, PF->pFuncNameEnd);
    break;
  case 'd':
    output(PF->pFuncNameEnd, PF->pPriorityKeyword);
    break;
  case 'A':
    output(PF->pPriorityEnd, PF->pDeclEnd);
    break;
  case 'F':
    output(PF->pFuncNameStart, PF->pFuncNameEnd);
    break;
  case 'P':
    output(PF->pPriorityStart, PF->pPriorityEnd);
    break;
  case 'l':
    fprintf(Fout, "%d", PF->lPriority);
    break;
  case 'f':
    fprintf(Fout, "\"%s\"", FileName);
    break;    
  default:
    failure("unknown template command");
    break;
  }
}

/* Output argument string
 */
static void outputarg(char c, int arg)
{
  switch ( c ) {
  case 'a':
    output(PF->a[arg].pNameStart, PF->a[arg].pNameEnd);
    break;
  case 'T':
    if ( !PF->a[arg].useVoidPointer ) {
      output(PF->a[arg].pDeclStart, PF->a[arg].pDeclEnd);
    } else {
      fprintf(Fout, " void *");
      output(PF->a[arg].pNameStart, PF->a[arg].pNameEnd);
    }
    break;
  }
}

/* Output code for a priority function based on content of PF structure.
 */
static void outputCode(void)
{
  char *c = Code, *c0;

  while ( 1 ) {
    char *c_bracket = c;
    int arg = -1;
    int ignore = 0;
    int repeat = 0;
    int line;

    c0 = c;
    while ( *c != '\n' ) {
      if ( *c != '%' ) {
	c++;
	continue;
      }
      if ( !ignore && c != c0 ) fwrite(c0, 1, c-c0, Fout);
      c++;

      if ( ignore && *c != 'b' ) {
	c0 = c;
	continue;
      }

      switch ( *c ) {
      case '%':
	fprintf(Fout, "%%");
	break;
      case 'n':
	if ( arg >= 0 && arg < PF->nargs ) {
	  if ( CR ) fprintf(Fout, "\r\n");
	  else fprintf(Fout, "\n");
	  if ( LineNumbering ) {
	    fprintf(Fout, "#line %d \"%s\"", PF->a[arg].lStart, FileName);
	    if ( CR ) fprintf(Fout, "\r\n");
	    else fprintf(Fout, "\n");
	  }
	}
	break;
      case 'B':
	if ( PF->nargs == 0 ) ignore = 1;
	else c_bracket = c;
	arg = 0;
	break;
      case 'b':
	ignore = 0;
	if ( repeat && ++arg < PF->nargs ) c = c_bracket;
	else {
	  arg = -1;
	  repeat = 0;
	}
	break;
      case 'T':
      case 'a':
	outputarg(*c, arg);
	repeat = 1;
	break;
      default:
	outputdef(*c);
	break;
      }

      c++;
      c0 = c;
    }

    if ( c != c0 ) fwrite(c0, 1, c-c0, Fout);

    c++;
    if ( c - CodeEnd >= 0 ) break;

    if ( CR ) fprintf(Fout, "\r\n");
    else fprintf(Fout, "\n");

    if ( c[0] == '%' && c[1] == 'D' )
      line = PF->lStart;
    else if ( !((c[0] == '\n') ||
		(c[0] == '\r' && c[1] == '\n') ||
		(c[0] == '%' && c[1] == 'B' && c[2] == '%' && c[3] == 'n' )) )
      line = PF->lEnd;
    else
      line = -1;
    if ( line >= 0 ) {
      if ( LineNumbering ) {
	fprintf(Fout, "#line %d \"%s\"", line, FileName);
	if ( CR ) fprintf(Fout, "\r\n");
	else fprintf(Fout, "\n");
      }
    }
  }
}

/* Compare strings without creating one. Comparison is done inside Input.
 * It returns the same value as strcmp would, since it internally calls
 * strcmp.
 */
static int comparestr(int start, int end, char *str)
{
  char save = Input[end];
  Input[end] = 0;
  int result = strcmp(Input+start, str);
  Input[end] = save;
  return result;
}

/* Symbol characters mapping
 */
static char symbolCharArray[256];

/* Init the symbol characters array
 */
static void symbolCharArrayInit(void)
{
  int i;
  for ( i = 0 ; i < sizeof(symbolCharArray) ; i++ )
    symbolCharArray[i] = 0;

  for ( i = '0' ; i <= '9' ; i++ )
    symbolCharArray[i] = 1;
  for ( i = 'A' ; i <= 'Z' ; i++ )
    symbolCharArray[i] = 1;
  for ( i = 'a' ; i <= 'z' ; i++ )
    symbolCharArray[i] = 1;

  symbolCharArray['_'] = 1;
}

/* Returns non zero if this is a symbol character
 */
static inline int isSymbolChar(unsigned char c)
{
  return symbolCharArray[c & 0xFF];
}

/* Increments state level and reinits all substates.
 * Note that once we start using substates, we cannot increase level.
 */
static void SMinc(int newstate)
{
  if ( ++Level >= N_LEVELS ) error("too many nested levels", 0);
  State[Level] = newstate;
  Substate = 0;
  Substate1 = 0;
  Phrase[Level] = 0;
  Symbol[Level] = -1;
}

/* Decrements state level and reinits all substates.
 * Note that once we start using substates, we cannot increase level.
 */
static void SMdec()
{
  // Note that State[0] should normally be equal to '\0' only?
  if ( --Level < 0 ) error("unmatched '%c'", State[0]);
  Substate = 0;
  Substate1 = 0;
}

/* Increment phrase number
 */
static void phraseinc(int level)
{
  Phrase[level]++;
  Symbol[level] = -1;
}

/* State machine that updates the current level we are in according to
 * character at new position.
 */
static void SMlevel(int position)
{
  char *c = Input + position;

  // Check if carriage return character is used in this file
  if ( c[0] == '\r' ) CR = 1;
  
  // Increment line number if end of line
  if ( position > 0 && c[-1] == '\n' ) {
    FinLine++;
    Line++;
  }

  // Check for start and end of C symbol or numeric integer
  SymbolStartStrobe = SymbolEndStrobe = 0;
  if ( isSymbolChar(c[0]) ) {
    if ( Symstate != 'a' ) {
      Symstate = 'a';
      Symbol[Level]++;
      SymbolStartStrobe = 1;
      SymbolStart = position;
    }
  } else {
    if ( Symstate == 'a' ) Symstate = 0;
  }
  if ( !isSymbolChar(c[1]) ) {
    if ( Symstate == 'a' ) SymbolEndStrobe = 1;
  }
  
  switch ( State[Level] ) {
  case '*':  // Comment /*
    if ( c[-1] == '*' && c[0] == '/' ) SMdec();
    break;
    
  case '/':  // Comment //
    if ( c[0] == '\n' ) SMdec();
    break;
    
  case '"':  // String
    if ( c[0] == '\n' ) error("incomplete string", 0);
    else if ( c[-1] != '\\' && c[0] == '"' ) SMdec();
    break;
    
  case '#':  // Preprocessor
    switch ( Substate ) {
    case '\\':  //  #\ .
      if ( c[0] == '\n' ) Substate = 0;
      break;

    case '"':  //  #"
    case '/':  //  #//
      switch ( Substate1 ) {
      case '\\':  //  #"\ .
	if ( c[0] == '\n' ) {
	  // Cancel substates but keep # state
	  Substate = 0;
	  Substate1 = 0;
	} else if ( isgraph(c[0]) )
	  Substate1 = 0;
	break;
      default:  //  #" or #//
	if ( c[0] == '\n' ) SMdec();
	else if ( Substate == '"' && c[0] == '"' ) Substate = 0;
	else if ( c[0] == '\\' ) Substate1 = '\\';
	break;
      }
      break;

    case '*':  //  #/*
      if ( c[-1] == '*' && c[0] == '/' ) Substate = 0;
      else if ( c[0] == '\n' ) {
	// Done with # state and we're in /* state
	State[Level] = '*';
	Substate = 0;
	Substate1 = 0;
	break;
      }
      break;
      
    default: //  #
      switch ( c[0] ) {
      case '*':
      case '/':
	if ( c[-1] == '/' ) Substate = c[0];
	break;
      case '"':
	Substate = '"';
	break;
      case '\\':
	Substate = '\\';
	break;
      case '\n':
	// Done with #
	SMdec();
	break;
      default:
	break;
      }
      break;
    }
    break;

  case '{':
  case '(':
  case '[':
  default:
    // Normal states with recursive levels
    switch ( c[0] ) {
    case '*':
      if ( c[-1] == '/' ) SMinc('*');
      break;
    case '/':
      if ( c[-1] == '/' ) SMinc('/');
      break;
    case '"':
      SMinc('"');
      break;
    case '#':
      // We could verify here if we have only white chars or /* */ comment
      // before #
      SMinc('#');
      break;
    case ';':
    case ',':
      phraseinc(Level);
      break;

    case '{':
      phraseinc(Level);
    case '(':
    case '[':
      SMinc(c[0]);
      break;

    case '}':
      if ( Level > 0 ) phraseinc(Level-1);
    case ')':
    case ']':
      if ( Level == 0 ) {
	printf("%s\n", Input+position-100);
	error("unmatched %d", position);
      }
      // In ASCII table we have consecutive () but not for [.] and {.}
      if ( State[Level]+1 != c[0] && State[Level]+2 != c[0] )
	error("unmatched '%c'", c[0]);
      SMdec();
      break;

    default:
      break;
    }
    break;
  }
}

/* Some priority functions declaration sanity checking
 */
void sanityCheck(void)
{
  if ( Code == NULL )
    error("missing preprocessor template "
	  "(make sure you include portos.h before any priority function declaration)", 0);
  if ( PF->pDeclStart < 0 )
    error("failed to identify start of function declaration "
	  "(make sure you specify return type as \"void\")", 0);
  if ( PF->pDeclEnd < PF->pDeclStart )
    error("failed to identify end of function declaration", 0);
  if ( PF->pFuncNameStart < PF->pDeclStart )
    error("failed to identify function name", 0);
  if ( PF->pFuncNameEnd < PF->pDeclStart )
    error("failed to identify function name", 0);
  if ( PF->pPriorityStart < PF->pDeclStart )
    error("failed to identify function's priority", 0);
  if ( PF->pPriorityEnd < PF->pDeclStart )
    error("failed to identify function's priority", 0);  
}

/* Main state machine
 *
 * Get the arguments as follows:
 * int a[5]
 * void (*a)(int b)
 * int *(*(*a)[7])(int[5][])
 * struct xxx *(*(*a)[7])[10][20]
 * There are 3 fields: the element type, the variable name and
 * the modifier (function or array).
 * The modifier can be present either inside block 2 or in block 3.
 * Arrays and function pointer are replaced by void* because their
 * declaration in function argument list differs from their declaration
 * outside: eg, "int a[5]" is a pointer in an argument list while it
 * is a vector allocation outside.
 * So we will identify the 3 fields, if present, determine if we
 * have a pointer, and find the argument name.
 * If the 3rd field is present, it must be a pointer (function or
 * array). If the second field is surrounded by parenthesis, we
 * need to locate the name inside: any name that is not enclosed
 * by [].
 * Any {}, comments, etc blocks are ignored.
 */
void SMmain(void)
{
  int position, pPrevious = 0;
  int arg = 0;
  int field = 0;
  int arrayPtr = 0;
  int pNameStart1 = 0, pNameEnd1 = 0, pNameStart2 = 0, pNameEnd2 = 0;
  int lineDirectiveOffset = 0; // To distinguish between #5 and #line 5
  int pLineDirectiveStart = 0, pLineDirectiveEnd = 0; // Line directive symbols
  int keywordStart = 0, keywordEnd = 0, instrLevel = 0;

  // States
  enum {
    NONE = 0,

    // Used by stateLine
    LINE_DIRECTIVE_GETNAME,

    // Used by state
    FUNCTION, FUNCTION_BLOCK_START, FUNCTION_BLOCK_START2,

    // Used by state1, state2
    PRIORITY, TIME_SIGNAL,
    BLOCK_START, BLOCK_END
  } state = NONE, state1 = NONE, state2 = NONE, stateLine = NONE;
  
  // Initializations that help in sanity checking
  PF->pDeclStart = -1;
  PF->pDeclEnd = -1;
  PF->pFuncNameStart = -1;
  PF->pFuncNameEnd = -1;
  PF->pPriorityStart = -1;
  PF->pPriorityEnd = -1;

  // We start from symbol number -1
  Symbol[0] = -1;

  // Mark first line
  if ( LineNumbering ) {
    fprintf(Fout, "#line %d \"%s\"", 1, FileName);
    if ( CR ) fprintf(Fout, "\r\n");
    else fprintf(Fout, "\n");
  }

  for ( position = 0 ; position < InputLen ; position++ ) {
    SMlevel(position);

    // Process #line directives to keep track of the file and line numbering
    if ( State[Level] == '#' && Substate == 0 && SymbolEndStrobe ) {
      // Check if #5 or #line
      if ( Symbol[Level] == lineDirectiveOffset &&
	   Input[SymbolStart] >= '0' &&
	   Input[SymbolStart] <= '9' ) {
	// Numeric line number
	char save = Input[position+1];
	Input[position+1] = 0;
	Line0 = atoi(Input + SymbolStart) - 1;
	Input[position+1] = save;
	lineDirectiveOffset = 0;
	stateLine = LINE_DIRECTIVE_GETNAME;
      }
      if ( Symbol[Level] == 0 && SymbolEndStrobe &&
	   !comparestr(SymbolStart, position+1, "line") ) {
	// #line directive, next symbol is numeric line number
	lineDirectiveOffset = 1;
      }
    }
    if ( State[Level] == '#' && Substate == '"' &&
	 stateLine == LINE_DIRECTIVE_GETNAME && pLineDirectiveStart == 0 ) {
      // Start of filename in #line directive
      pLineDirectiveStart = position + 1;
    }
    if ( State[Level] == '#' && Substate != '"' &&
	 stateLine == LINE_DIRECTIVE_GETNAME &&
	 pLineDirectiveStart != 0 && pLineDirectiveEnd == 0 ) {
      // Start of filename in #line directive
      char save;
      pLineDirectiveEnd = position;
      if ( pLineDirectiveEnd - pLineDirectiveStart >= sizeof(FileName) )
	error("#line directive has file name too long", 0);
      Line = Line0;
      save = Input[pLineDirectiveEnd];
      Input[pLineDirectiveEnd] = 0;
      strncpy(FileName, Input + pLineDirectiveStart, sizeof(FileName)-1);
      Input[pLineDirectiveEnd] = save;
      stateLine = NONE;
      pLineDirectiveStart = 0;
      pLineDirectiveEnd = 0;
    }

    // Detect preprocessor template
    if ( Code == NULL ) {
      if ( Level == 0 && SymbolEndStrobe &&
	   !comparestr(SymbolStart, position+1, "po_prep_template_start") ) {
	// Start of template
	output(pPrevious, SymbolStart - 1);
	pPrevious = position + 1;
      } else if ( Level == 0 && SymbolEndStrobe &&
		  !comparestr(SymbolStart, position+1, "po_prep_template_start_TI") ) {
	// Start of template
	output(pPrevious, SymbolStart - 1);
	pPrevious = position + 1;
	TI = 1;
      }
      if ( Level == 0 && SymbolEndStrobe &&
	   !comparestr(SymbolStart, position+1, "po_prep_template_end") ) {
	Code = Input + pPrevious;
	CodeEnd = Input + SymbolStart;
	pPrevious = position + 1;
      }
      // Until template is found, don't look for priority functions
      continue;
    }

    // Check preprocessor version
    Checkmode = 4;    // Disabled
    if ( Checkmode < 4 ) {
      if ( Checkmode == 0 && Level == 0 && SymbolEndStrobe &&
	   !comparestr(SymbolStart, position+1, "po_signal_creategroup") ) {
	Checkmode = 1;
      }
      if ( Checkmode == 2 && Level == 0 && SymbolEndStrobe &&
	   !comparestr(SymbolStart, position+1, "po_init") ) {
	Checkmode = 3;
      }
      if ( Checkmode == 1 && Level == 0 && Input[position] == '}' ) {
	Checkmode = 2;
      }
      if ( Checkmode == 3 && Level == 0 && Input[position] == '}' ) {
	Checkmode = 4;
	//printf("************* Checkver = 0x%x\n", Checkver);
	if ( (!TI && Checkver != 0x0) || (Checkver != 0xc5214b7e) ) {
	  FinLine = Line = 1;
	  strcpy(FileName, FinName);
	  failure("wrong Portos preprocessor version");
	}
      }
      if ( (Checkmode == 1 || Checkmode == 3) && isgraph(Input[position]) ) {
	int shift = Checkver & 0xF;
	if ( shift > 26 ) shift = 26;
	Checkver += Input[position] << shift;
      }
    }

    if ( state == FUNCTION_BLOCK_START || state == FUNCTION_BLOCK_START2 ) {
      // Looking for priority function block start

      if ( Level == 0 && Input[position] == ';' ) {
	// We're done with function args
	// That was just a prototype, ignore
	PF = PF_Vec;
	state = NONE;
      }
      if ( Level == 1 && Input[position] == '{' ) {
	// Found priority function block, output code
	int count = PF - PF_Vec;
	PF = PF_Vec;

	// Output pf code
	for ( ; count > 0 ; count-- ) {
	  // Output until start of declaration
	  FoundPF = 1;
	  sanityCheck();
	  output(pPrevious, position);
	  outputCode();
	  pPrevious = PF->pDeclEnd;
	  PF++;
	}
	PF = PF_Vec;
	state = NONE;
      }
    }

    if ( state == NONE ||
	 state == FUNCTION_BLOCK_START || state == FUNCTION_BLOCK_START2 ) {
      // Searching for the start of a priority function (0), or priority
      // function block start (-1, -2).
      if ( (Level == 0 && Symbol[Level] == 0 && SymbolStartStrobe) ||
	   (state == FUNCTION_BLOCK_START && Level == 0 &&
	    SymbolStartStrobe) ) {
	// This could be the start of a function declaration, remember
	// position
	if ( PF - PF_Vec >= N_CONTEXTS )
	  error("too many different declarations of priority function", 0);
	PF->pDeclStart = position;
	PF->lStart = Line;
	if ( state == FUNCTION_BLOCK_START )
	  state = FUNCTION_BLOCK_START2;
      }
      if ( Level == 0 && Symbol[Level] > 0 && SymbolEndStrobe ) {
	// This could be the end of a function name
	PF->pFuncNameStart = SymbolStart;
	PF->pFuncNameEnd = position + 1;
      }
      if ( Level == 1 && State[1] == '(' &&
	   Phrase[1] == 0 && Symbol[1] == 0 &&
	   SymbolEndStrobe ) {
	// This could be our symbol of interest
	if ( !comparestr(SymbolStart, position+1, "po_priority") ) {
	  // this is a priority function
	  state = FUNCTION;
	  arg = -1;
	  PF->lPriority = Line;
	  PF->pPriorityKeyword = SymbolStart;
	}
      }
    }

    if ( state == FUNCTION ) {
      // Processing priority function + arguments
      
      if ( Level == 2 && Phrase[1] == 0 && Input[position] == '(' ) {
	// Get the priority value
	PF->pPriorityStart = position;
      }
      if ( Level == 1 && Phrase[1] == 0 && Input[position] == ')' ) {
	// End of priority
	PF->pPriorityEnd = position + 1;
      }
      if ( (Level == 1 && Input[position] == ',') ||
	   (Level == 0 && Input[position] == ')') ) {
	// End of previous argument
	arg = Phrase[1] - 2;
	if ( Level == 0 ) arg++;
	if ( arg >= 0 ) {
	  if ( pNameStart2 >= 0 ) {
	    PF->a[arg].pNameStart = pNameStart2;
	    PF->a[arg].pNameEnd = pNameEnd2;
	  } else if ( pNameStart1 >= 0 ) {
	    PF->a[arg].pNameStart = pNameStart1;
	    PF->a[arg].pNameEnd = pNameEnd1;
	  } else {
	    error("unrecognized argument %d", arg-1+1);
	  }
	  PF->a[arg].pDeclEnd = position;
	  // Use a void pointer?
	  PF->a[arg].useVoidPointer = (field == 3 && arrayPtr);
	}

	if ( Level == 1 ) {
	  // Start of new argument
	  arg = Phrase[1] - 1;
	  pNameStart1 = -1;
	  pNameStart2 = -1;
	  field = 1;
	  arrayPtr = 0;
	  PF->a[arg].lStart = 0;
	} else {
	  // End of declaration
	  PF->pDeclEnd = position + 1;
	  PF->nargs = arg + 1;
	  if ( PF->nargs > N_ARGS ) error("too many function arguments", 0);
	  PF->lEnd = Line;
	  PF++;
	  state = FUNCTION_BLOCK_START;
	}
      }
      if ( arg >= 0 && PF->a[arg].lStart <= 0 &&
	   Input[position] != ','  && !isgraph(Input[position]) ) {
	PF->a[arg].lStart = Line;
	PF->a[arg].pDeclStart = position;
      }
      if ( Level == 1 && SymbolEndStrobe ) {
	// Could be our variable name in field 1
	pNameStart1 = SymbolStart;
	pNameEnd1 = position + 1;
      }
      if ( Level == 2 && State[Level] == '(' && Input[position] == '(' ) {
	// Update argument's field number
	if ( ++field > 3 ) field = 3;
      }
      if ( field < 3 && Level > 1 && State[Level] == '[' ) {
	field = 3;
	arrayPtr = 1;
      }
      if ( Level > 1 && State[Level] == '(' && field == 2 &&
	   SymbolEndStrobe ) {
	// Could be our variable name in field 2
	pNameStart2 = SymbolStart;
	pNameEnd2 = position + 1;
      }
    }

    // Replace po_priority(), po_time(), po_signal()

    if ( state2 == BLOCK_START && Level <= instrLevel &&
	 isgraph(Input[position]) ) {
      // Ignore this po_priority without "(P)"
      // NOTE: we need the "else if" above to prevent undoing the previous
      // "if" right away
      state1 = NONE;
      state2 = NONE;
    }
    if ( Level >= 1 && SymbolEndStrobe &&
	 !comparestr(SymbolStart, position+1, "po_priority") ) {
	// This is currently handled separately from the priority functions
	// above
      state1 = PRIORITY;
      state2 = BLOCK_START;
      keywordStart = SymbolStart;
      keywordEnd = position + 1;
      instrLevel = Level;
    }
    if ( Level >= 1 && SymbolEndStrobe &&
	 (!comparestr(SymbolStart, position+1, "po_signal") ||
	  !comparestr(SymbolStart, position+1, "po_time") ||
	  !comparestr(SymbolStart, position+1, "po_signalp") ||
	  !comparestr(SymbolStart, position+1, "po_timep")) ) {
	// This is currently handled separately from the priority functions
	// above
      state1 = TIME_SIGNAL;
      state2 = BLOCK_START;
      keywordStart = SymbolStart;
      keywordEnd = position + 1;
      instrLevel = Level;
    }
    if ( state2 == BLOCK_START && Level == instrLevel+1 &&
	 Input[position] == '(' ) {
      // Found block (), locate end of block
      state2 = BLOCK_END;
    }
    if ( state1 == TIME_SIGNAL && state2 == BLOCK_END &&
	 Level == instrLevel+1 && Input[position+1] == ')' ) {
      // Found matching ')'
      // Replace directive with appropriate declaration
      if ( Phrase[Level] >= 2 ) {
	output(pPrevious, keywordEnd);
	fprintf(Fout, "_h");
	pPrevious = keywordEnd;
      }
      state1 = NONE;
      state2 = NONE;
    }
    if ( state1 == PRIORITY && state2 == BLOCK_END &&
	 Level == instrLevel && Input[position] == ')' ) {
      // Found matching ')'
      // Replace directive with appropriate declaration
      output(pPrevious, keywordStart);
      fprintf(Fout, "po_function_ServiceHandle *po__srvhandle");
      pPrevious = position+1;
      state1 = NONE;
      state2 = NONE;
      if ( Level > 1 ) {
	// Fix input buffer too in case we copy arguments
	sprintf(Input+keywordStart, "po_funcSrvH*%*s",
		position-keywordStart-12, "");
	Input[position] = ' ';
      }
    }

    // NOTE: after this point, due to the last statement above,
    // Input[position] may have changed value from ')' to ' '

  }

  if ( FoundPF && Checkmode < 4 ) {
    FinLine = Line = 1;
    strcpy(FileName, FinName);
    failure("wrong Portos preprocessor version");
  }

  output(pPrevious, position);
  #if 0
  if ( versionProblem ) {
    printf("  ERROR: Portos source files and po_preprocess versions don't match.\n");
    exit(1);
  }
  #endif
}

static void checkForCR(void)
{
  int i;
  for ( i = 1 ; i < InputLen ; i++ ) {
    if ( Input[i] == '\n' ) {
      if ( Input[i-1] == '\r' || Input[i+1] == '\r' )
	CR = 1;
      break;
    }
  }
}

/* Main loop
 */
static void mainLoop(char *inputName, char *outputName)
{
  strncpy(FileName, inputName, sizeof(FileName)-1);
  strncpy(FinName, inputName, sizeof(FinName)-1);
  FoutName = outputName;

  Fin = fopen(inputName, "rb");
  if ( !Fin ) failure("can't open input file");
  Fout = fopen(outputName, "wb");
  if ( !Fout ) failure("can't open output file");

  fseek(Fin, 0, SEEK_END);
  InputLen = ftell(Fin);
  fseek(Fin, 0, SEEK_SET);

  Input = (char*)malloc(InputLen+1);
  if ( !Input ) failure("can't allocate memory for input file");
  fread(Input, 1, InputLen, Fin);
  fclose(Fin);
  
  Input[InputLen] = 0; // To avoid issues with /*, //, \", ... at end of file

  symbolCharArrayInit();
  checkForCR();

  SMmain();

  free(Input);
  fclose(Fout);
}

static void usageHelp(void)
{
  message("  usage: po_preprocess [options] input_file output_file");
  message("             -nl\tno line numbering (#line)");
  message("            --help\tthis help message");
}

/* main
 */
int main(int argc, char **argv)
{
  int i;

  if ( argc < 3 ) {
    usageHelp();
    exit(1);
  }

  for ( i = 1 ; i < argc-2 ; i++ ) {
    if ( !strcmp(argv[i], "-nl") ) {
      LineNumbering = 0;
    } else if ( !strcmp(argv[i], "--help") ) {
      usageHelp();
      exit(1);
    } else {
      char msg[1000];
      sprintf(msg, "unrecognized option %s", argv[i]);
      failure(msg);
    }
  }

  if ( !strcmp(argv[argc-2], argv[argc-1]) ) {
    failure("input and output files cannot be identical");
  }

  mainLoop(argv[argc-2], argv[argc-1]);
  return 0;
}
