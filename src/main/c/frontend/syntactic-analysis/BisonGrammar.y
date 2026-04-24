%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 *
 * @todo Add location to the grammar and "pushToken" API function.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	signed int integer;
	bool boolean;
	char * string;
	TokenLabel token;

	/** Non-terminals. */

	ChordNoteList * chordNoteList;
	DurationType duration;
	Event * event;
	EventList * eventList;
	Expression * expression;
	GlobalSetting * globalSetting;
	GlobalSettingList * globalSettingList;
	ModeType mode;
	Program * program;
	Track * track;
	TrackList * trackList;
	VarType varType;
}

/**
 * Destructors. These functions are executed when a symbol is discarded
 * (e.g., during error recovery). The program non-terminal is excluded
 * because the AST must persist after parsing.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { destroyChordNoteList($$); } <chordNoteList>
%destructor { destroyEvent($$); } <event>
%destructor { destroyEventList($$); } <eventList>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyGlobalSetting($$); } <globalSetting>
%destructor { destroyGlobalSettingList($$); } <globalSettingList>
%destructor { destroyTrack($$); } <track>
%destructor { destroyTrackList($$); } <trackList>

/** Terminals - literals. */
%token <integer> INTEGER
%token <string> STRING
%token <string> PITCH
%token <string> NOTE_CLASS
%token <string> IDENTIFIER

/** Terminals - music keywords. */
%token <token> TEMPO
%token <token> TIME_SIGNATURE
%token <token> KEY
%token <token> MAJOR
%token <token> MINOR
%token <token> TRACK
%token <token> INSTRUMENT
%token <token> PLAY
%token <token> REST
%token <token> VELOCITY
%token <token> REPEAT
%token <token> IF
%token <token> THEN
%token <token> ELSE

/** Terminals - type keywords. */
%token <token> INTEGER_TYPE
%token <token> BOOLEAN_TYPE
%token <token> STRING_TYPE
%token <token> TRUE
%token <token> FALSE

/** Terminals - logical operators (as keywords). */
%token <token> AND
%token <token> OR
%token <token> NOT

/** Terminals - duration keywords. */
%token <token> WHOLE
%token <token> HALF
%token <token> QUARTER
%token <token> EIGHTH
%token <token> SIXTEENTH

/** Terminals - operators. */
%token <token> ADD
%token <token> SUB
%token <token> MUL
%token <token> DIV
%token <token> LT
%token <token> GT
%token <token> EQ
%token <token> NEQ
%token <token> LEQ
%token <token> GEQ

/** Terminals - punctuation. */
%token <token> SEMICOLON
%token <token> OPEN_BRACE
%token <token> CLOSE_BRACE
%token <token> OPEN_BRACKET
%token <token> CLOSE_BRACKET
%token <token> COMMA

%token <token> IGNORED
%token <token> UNKNOWN
%token <token> OPEN_COMMENT
%token <token> CLOSE_COMMENT

/** Non-terminals. */
%type <chordNoteList> chordNoteList
%type <duration> duration
%type <event> event
%type <eventList> eventList
%type <eventList> optElse
%type <expression> expression
%type <globalSetting> globalSetting
%type <globalSettingList> globalSettingList
%type <mode> mode
%type <program> program
%type <track> track
%type <trackList> trackList
%type <varType> varType

/**
 * Precedence and associativity (lowest to highest).
 *
 * @see https://en.cppreference.com/w/cpp/language/operator_precedence.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%left OR
%left AND
%right NOT
%left LT GT EQ NEQ LEQ GEQ
%left ADD SUB
%left MUL DIV

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program: globalSettingList trackList					{ $$ = ProgramSemanticAction($1, $2); }
       ;

globalSettingList: globalSettingList globalSetting		{ $$ = AppendGlobalSettingSemanticAction($1, $2); }
                 | %empty								{ $$ = NULL; }
                 ;

globalSetting: TEMPO INTEGER SEMICOLON						{ $$ = TempoSettingSemanticAction($2); }
             | TIME_SIGNATURE INTEGER DIV INTEGER SEMICOLON	{ $$ = TimeSignatureSettingSemanticAction($2, $4); }
             | KEY NOTE_CLASS mode SEMICOLON				{ $$ = KeySettingSemanticAction($2, $3); }
             ;

mode: MAJOR    { $$ = MODE_MAJOR; }
    | MINOR    { $$ = MODE_MINOR; }
    ;

trackList: trackList track									{ $$ = AppendTrackSemanticAction($1, $2); }
         | track											{ $$ = SingleTrackSemanticAction($1); }
         ;

track: TRACK IDENTIFIER INSTRUMENT STRING OPEN_BRACE eventList CLOSE_BRACE
           { $$ = TrackSemanticAction($2, $4, $6); }
     ;

eventList: eventList event									{ $$ = AppendEventSemanticAction($1, $2); }
         | %empty											{ $$ = NULL; }
         ;

event: PLAY PITCH duration SEMICOLON
           { $$ = PlayNoteSemanticAction($2, $3, NULL); }
     | PLAY PITCH duration VELOCITY expression SEMICOLON
           { $$ = PlayNoteSemanticAction($2, $3, $5); }
     | PLAY OPEN_BRACKET chordNoteList CLOSE_BRACKET duration SEMICOLON
           { $$ = PlayChordSemanticAction($3, $5, NULL); }
     | PLAY OPEN_BRACKET chordNoteList CLOSE_BRACKET duration VELOCITY expression SEMICOLON
           { $$ = PlayChordSemanticAction($3, $5, $7); }
     | REST duration SEMICOLON
           { $$ = RestEventSemanticAction($2); }
     | varType IDENTIFIER EQ expression SEMICOLON
           { $$ = VarDeclEventSemanticAction($1, $2, $4); }
     ;

chordNoteList: chordNoteList COMMA PITCH					{ $$ = AppendChordNoteSemanticAction($1, $3); }
             | PITCH										{ $$ = SingleChordNoteSemanticAction($1); }
             ;

duration: WHOLE       { $$ = DURATION_WHOLE; }
        | HALF        { $$ = DURATION_HALF; }
        | QUARTER     { $$ = DURATION_QUARTER; }
        | EIGHTH      { $$ = DURATION_EIGHTH; }
        | SIXTEENTH   { $$ = DURATION_SIXTEENTH; }
        ;

varType: INTEGER_TYPE    { $$ = VAR_INTEGER; }
       | BOOLEAN_TYPE    { $$ = VAR_BOOLEAN; }
       | STRING_TYPE     { $$ = VAR_STRING; }
       ;

%%
