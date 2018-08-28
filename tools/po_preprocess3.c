// Definitions
enum {
  // Terminals
  TERMINALS = 1000,
  IDENTIFIER,
  NUMERAL,        // for line numbers (and for speed by avoiding digits)
  STRING_LITERAL, // "..."
  CHAR_LITERAL,   // '...'

  PREPROCESSOR,
};

int rule_argument1[] = {
  SKIP, RECORD, IDENTFIER, STARTING_LEVEL, ','
};

int rule_argument2[] = {
  SKIP, RECORD, IDENTFIER, STARTING_LEVEL, ')'
};

int rule_arguments[] = {
  REPEAT, ARGUMENT, UNTIL, ')'
};

int rule_attributes[] = {
  REPEAT, IDENTIFIER, '(', ARGUMENTS, ')', END
};

int rule_level0_declaration1[] = {
  LEVEL1, '}', ANY   // Action will reject rule if ANY is ';'
};

int rule_level0_declaration2[] = {
  LEVEL0, ';', ANY   // Action will reject rule if ANY is ';'
};

// e.g.: static void func(po_priority(...), ...) __attribute__(...) {
int rule_po_priority[] = {
  IDENTIFIER, SKIP, PO_PRIORITY, '(', ARGUMENTS, ')',
  ARGUMENTS, ')', ATTRIBUTES, ANY
};

// e.g.: po_signal(5,, &handle)
int rule_po_signal[] = {
  PO_SIGNAL, '(', ARGUMENTS, ')'
};

// e.g.: po_time(123, &clock)
int rule_po_time[] = {
  PO_TIME, '(', ARGUMENTS, ')'
};

// String type: pointers to start and end of string (it can be converted
// to normal C string for printing)
typedef struct {
  char *start;
  char *end;
} MyString;

// Type for each char
static char typeT[256];

// Buffer that contains entire input file
static char *input;
static char *input_end; // Points at EOF (followed by '\n')

// File name and line
static char *filename0, *filename; // original name and current name
static int line0 = 1, line = 1; // Original line and current line

// Global pointers to where we are in the file
static char *yytext0, *yytext; // Start and end of token

// Lexer that returns tokens (one by one) and updates yytext, yytext0
// It assumes a '\n' is inserted after EOF (that stops inner loops)
int lexer(void)
{
  char *p = yytext;  
  yy_text0 = yytext;
  int type;
  
  // Non interesting char have type ' '. Skip them
  while ( typeT[*p] == ' ' ) p++;

  // Check for EOF
  if ( p == input_end ) {
    yytext = p;
    return 0; // EOF
  }

  type = typeT[*p];
  p++;

  switch ( type ) {
  case STRING_LITERAL: // "..."
    while ( (*p != '\"' || p[-1] == '\\') && *p != '\n' ) p++;
    if ( *p == '\n' ) error("unterminated string literal", 0);
    break;

  case CHAR_LITERAL: // '...' 
    while ( (*p != '\'' || p[-1] == '\\') && *p != '\n' ) p++;
    if ( *p == '\n' ) error("unterminated char literal", 0);
    break;

  case IDENTIFIER: // Starts with [A-Za-z_] but can then also contain [0-9]
    while ( typeT[*p] == IDENTIFIER || typeT[*p] == NUMERAL ) p++;
    break;

  case NUMERAL: // Starts with [0-9] but can then also contain [A-Za-z_]
                // We only care for line numbers here (no signs, no floats)
    while ( typeT[*p] == IDENTIFIER || typeT[*p] == NUMERAL ) p++;
    break;

  case '\n': // Token useful to C preprocessor's #line directive
    line0++;
    line++;
    break;
  }

  yytext = p;
  return type;
}

// Preprocessor that handles # line directives
int c_preprocessor(int type)
{
  enum {NONE, LINENUM, FILENAME, END};
  static int state = NONE; // Waiting for some token
  int result = PREPROCESSOR;

  if ( type == '\n' ) {
    state = NONE;
    result = 0;
  } else if ( state == NONE && type == '#' ) {
    state = LINENUM;
  } else if ( state == LINENUM && type == NUMERAL ) {
    line = atoi(yytext0); // It should work as it terminates with non-digit!!!
    state = FILENAME;
  } else if ( state == FILENAME && type == STRING_LITERAL ) {
    mystrcpy(yytext0+1, yytext-1, filename);
    state = END;
  }
  return result;
}

// Grammar structure
// Nested levels: ( [ {
static int level;
static int level_type[LEVELS];  // Type of each level: ( [ {

// Remember previous information
static int prev_type;
static char *prev_yytext;

// Declaration at level 0: function declaration
static MyString decl;

// List of arguments at currently specified level
static int arg_level = -1;
static int argc = 0;  // Argument counter
static MyString arg[ARGS];

// Remember last identifier per level
static MyString identifier[LEVELS];

// Level increment ( [ {
static void level_incr(int type)
{
  switch ( type ) {
  case '(':
  case '[':
  case '{':
    level++; // Jump to next level
    if ( level >= LEVELS ) error("too many nested levels");
    level_type[level] = type;
    break;
  }
}

// Level decrement ) ] }
static void level_incr(int type)
{
  char match;

  switch ( type ) {
  case ')':
  case ']':
  case '}':
    match = type - 1 - (type > ')'); // Matching ( [ {
    if ( level <= 0 || level_type[level] != match )
      error("unmatched %c", match);
    level--; // Back to previous level
    break;
  }
}

// Catch function declaration start (call before level_incr)
static void func_decl(int type)
{
  // Check for declaration start
  if ( level == 0 ) {
    switch ( prev_type ) {
    case '}':
    case ';':
      // New function declaration
      decl.start = yytext0;
      break;
    }
  }
}

// Argument declaration start, at given level (call after level_incr)
static void arg_decl(int type, int at_level)
{
  if ( level == at_level ) {
    switch ( prev_type ) {
    case '(':
    case ',':
      // New argument declaration
      arg.start[at_level] = yytext0;
      break;
    }
  }

// Argument declaration end, at given level (call before level_decr)
static void arg_decl(int type, int at_level)
 
  if ( level == at_level-1 && type == ')' ||
       level == at_level && type == ',' ) {
    // End of argument declaration
    arg.end[at_level] = prev_yytext;
    break;
  }
}

// Remember last identifier per level
static void last_identifier(int type)
{
  if ( type == IDENTIFIER ) {
    identifier[level].yystart = yytext0;
    identifier[level].yyend = yytext;    
  }
}

  // Remember types for next call
  prev_type = type;
  prev_yytext = yytext;
