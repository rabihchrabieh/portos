// Definitions
enum {
  // Terminals
  TERMINALS = 1000,
  IDENTIFIER,
  NUMERAL,        // for line numbers (and for speed by avoiding digits)
  STRING_LITERAL, // "..."
  CHAR_LITERAL,   // '...'

  PREPROCESSOR,

  // Nonterminals
  NONTERMINALS = 2000,

  PO_PRIORITY,
  PO_SIGNAL,
  PO_TIME,

  ARGUMENT,
  ARGUMENTS,
  ATTRIBUTES,
  RULE_PO_PRIORITY,
  RULE_PO_SIGNAL,
  RULE_PO_TIME,
  RULE_DECLARATION,

  // State machine (part of nonterminals, to simplify and speed up grammar)
  // They act like attributes or conditions to following rules
  STATEMACHINE = 3000,
  REPEAT,            // Repeat following symbols, 0 or more times
  END,               // Ends repeated list
  UNTIL,             // Ends repeated list and repeats conditionally
  SKIP,              // a SKIP b (a != b or a empty): skip tokens != from a,b
  RECORD,            // RECORD a: in skip mode, catch all tokens a
  ANY,               // Any token (one token)
  LEVEL0,            // Level 0 is outside any {}, [], or ()
  LEVEL1,            // Level 1 is inside 1 level of {}, [], or ()
  STARTING_LEVEL,    // Level at which rule has started
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

typedef struct {
  int type;
  char *yystart;
  char *yyend;
  int level;
  int level_type; // {}, [], ()
  int line;       // line number
} Symbol;

// Type for each char
static char typeT[256];

// Buffer that contains entire input file
static char *input;
static int inputlen;

// File name and line
static char *filename0, *filename; // original name and current name
static int line0 = 1, line = 1; // Original line and current line

// Global pointers to where we are in the file
static char *yytext0, *yytext; // Previous position and new position

// Lexer that returns tokens (one by one) and updates yytext, yytext0
int lex(void)
{
  char *p = yytext;  
  yy_text0 = yytext;
  int type;
  
  // Non interesting char have type ' '. Skip them
  do {
    while ( typeT[*p] == ' ' ) p++;
    if ( p - input >= inputlen ) {
      yytext = p;
      return 0; // EOF
    }
  } while ( *p == 0 ); // Also skip '\0' in middle of file

  type = typeT[*p];
  p++;

  switch ( type ) {
  case STRING_LITERAL: // "..."
    while ( (*p != '"' || p[-1] == '\\') && *p != '\n' && *p != 0 ) p++;
    if ( *p == 0 || *p == '\n' ) error("unterminated string literal", 0);
    break;
      
  case CHAR_LITERAL: // '..."
    while ( (*p != '\'' || p[-1] == '\\') && *p != '\n' && *p != 0 ) p++;
    if ( *p == 0 || *p == '\n' ) error("unterminated character literal", 0);
    break;

  case IDENTIFIER: // Starts with [A-Za-z_] but can then contain [0-9]
    while ( typeT[*p] == IDENTIFIER || typeT[*p] == NUMERAL ) p++;
    break;

  case NUMERAL: // Starts with [0-9] but can then contain [A-Za-z_]
                // We only care for line numbers here (no signs, no floats)
    while ( typeT[*p] == IDENTIFIER || typeT[*p] == NUMERAL ) p++;
    break;

  case '\n':
    line++;
    break;

    default:
      break;
  }

  yytext = p;
  return type;
}

// Preprocessor that handles # line directives
int prep(int type)
{
  static int state = 0;

  if ( type == '\n' ) {
    state = 0;
    return 0;
  }

  if ( state == 0 && type == NUMERAL ) {
    line = atoi(yytext0); // It should work as it terminates with non-digit!!!
    state = 1;
  } else if ( state = 1 && type == STRING_LITERAL ) {
    mystrcpy(yytext0+1, yytext-1, filename);
    state = 2;
  }
  return PREPROCESSOR;
}

// Level: (, [, {
int level = 0;
int level_type[LEVELS];
int level_id[LEVELS]; // Numbering of the level above, e.g. {() () [] {} ()}

// Increment level
static void level_incr(int type)
{
  switch ( type ) {
  case '(':
  case '[':
  case '{':
	level_id[level]++;
    level++;
    level_type[level] = type;
    break;
  }
}

// Decrement level
static void level_decr(int type)
{
  char match;

  switch ( type ) {
  case ')':
  case ']':
  case '}':
    match = type - 1 - (type > ')');
    if ( level <= 0 || level_type[level] != match ) error("unmatched %c", match);
    level_id[level] = 0; // To reset or not to reset?
    level--;
    break;
  }
}

static int rule_repeat(int type, Context *r)
{
  r->repeat = r->index = r->index + 1;
  return 1;
}

static int rule_repeat_end(int type, Context *r)
{
  r->index = r->repeat;
  return 1;
}

static int rule_repeat_until(int type, Context *r)
{
  int i = r->index + 1;
  if ( r->type[i] == type ) {
    r->index = i;
    return 1;
  } else {
    return rule_repeat_end(type, r);
  }
}

// Starting rules are pushed onto active rules for each new token.
// Active rules array is compacted by removing failed rules at each
// new token (or once we have more than N failed rules?). Subrules are
// also pushed onto active rules array.
Rule start_rules[NN];
Rule active_rules[NNN];

static int grammar(int type)
{
  Rule *r;

  // Compact active rules then push start rules
  ...;

  // Check each rule in active array
  for ( r = active_rules ; *r != NULL ; r++ ) {
    int i = r->index, tt;
    if ( i < 0 ) continue; // This is a failed rule
    tt = r->type[i];

    while ( tt > STATEMACHINE ) {
      
    }
  }

    {

    // Execute nonterminals (sub-rules)
    while ( r->type[i] > NONTERMINALS ) {
      if ( !(call_rule[r->type[i] - NONTERMINALS - 1](r)) ) {
	// Rule failed
	goto failed;
      }
      i = r->index;
      if ( r->wait ) {
	// Wait for recursive rule to finish
	goto wait;
      }
    }

    

    if ( r->type[i] == REPEAT ) r->repeat = i + 1;
    else if ( r->type[i] == REPEAT_END ) i = r->repeat;

    if ( r->repeat > 0 && r->type[i] != type ) {
      // Done with repetition command
      i = r->index + 1;
      r->repeat = 0;
      if ( i >= r->length ) {
	// Rule is successful
	goto success;
      }
    } else if ( r->type[i] != type ) {
      // Failure, reset rule
      r->index = 0;
      continue;
    } else if ( r->type[i] == type ) {
      if ( ++i >= r->length ) {
	// Rule is successful
	goto success;
      }
    }

    r->index = i;
    return 0;

  success:
    r->index = 0;
    r->func(r->context);
    return 1;
  }
}

