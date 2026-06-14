%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus LeaveMultilineCommentLexemeAction();
extern CompilationStatus NoteClassLexemeAction();
extern CompilationStatus OperatorLexemeAction(TokenLabel label);
extern CompilationStatus PitchLexemeAction();
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);
extern CompilationStatus StringLexemeAction();
extern CompilationStatus UnknownLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"//"[^\n]*								{ return IgnoredLexemeAction(); }

"/*"									{ return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"					{ return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+				{ return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.					{ return IgnoredLexemeAction(); }

"tempo"									{ return KeywordLexemeAction(TEMPO); }
"time"									{ return KeywordLexemeAction(TIME); }
"signature"								{ return KeywordLexemeAction(SIGNATURE); }
"key"									{ return KeywordLexemeAction(KEY); }
"major"									{ return KeywordLexemeAction(MAJOR); }
"minor"									{ return KeywordLexemeAction(MINOR); }
"track"									{ return KeywordLexemeAction(TRACK); }
"instrument"							{ return KeywordLexemeAction(INSTRUMENT); }
"play"									{ return KeywordLexemeAction(PLAY); }
"rest"									{ return KeywordLexemeAction(REST); }
"velocity"								{ return KeywordLexemeAction(VELOCITY); }
"repeat"								{ return KeywordLexemeAction(REPEAT); }
"if"									{ return KeywordLexemeAction(IF); }
"then"									{ return KeywordLexemeAction(THEN); }
"else"									{ return KeywordLexemeAction(ELSE); }
"integer"								{ return KeywordLexemeAction(INTEGER_TYPE); }
"boolean"								{ return KeywordLexemeAction(BOOLEAN_TYPE); }
"string"								{ return KeywordLexemeAction(STRING_TYPE); }
"true"									{ return KeywordLexemeAction(TRUE); }
"false"									{ return KeywordLexemeAction(FALSE); }
"AND"									{ return KeywordLexemeAction(AND); }
"OR"									{ return KeywordLexemeAction(OR); }
"NOT"									{ return KeywordLexemeAction(NOT); }
"whole"									{ return KeywordLexemeAction(WHOLE); }
"half"									{ return KeywordLexemeAction(HALF); }
"quarter"								{ return KeywordLexemeAction(QUARTER); }
"eighth"								{ return KeywordLexemeAction(EIGHTH); }
"sixteenth"								{ return KeywordLexemeAction(SIXTEENTH); }

"!="									{ return OperatorLexemeAction(NEQ); }
"<="									{ return OperatorLexemeAction(LEQ); }
">="									{ return OperatorLexemeAction(GEQ); }
"+"										{ return OperatorLexemeAction(ADD); }
"-"										{ return OperatorLexemeAction(SUB); }
"*"										{ return OperatorLexemeAction(MUL); }
"/"										{ return OperatorLexemeAction(DIV); }
"<"										{ return OperatorLexemeAction(LT); }
">"										{ return OperatorLexemeAction(GT); }
"="										{ return OperatorLexemeAction(EQ); }

";"										{ return PunctuationLexemeAction(SEMICOLON); }
"{"										{ return PunctuationLexemeAction(OPEN_BRACE); }
"}"										{ return PunctuationLexemeAction(CLOSE_BRACE); }
"["										{ return PunctuationLexemeAction(OPEN_BRACKET); }
"]"										{ return PunctuationLexemeAction(CLOSE_BRACKET); }
","										{ return PunctuationLexemeAction(COMMA); }
"("										{ return PunctuationLexemeAction(OPEN_PAREN); }
")"										{ return PunctuationLexemeAction(CLOSE_PAREN); }

[A-G][#bn]?[0-9]						{ return PitchLexemeAction(); }
[A-G][#b]?								{ return NoteClassLexemeAction(); }

[[:digit:]]+							{ return IntegerLexemeAction(); }
\"[^\"]*\"								{ return StringLexemeAction(); }
[a-z_][a-zA-Z0-9_]*					{ return IdentifierLexemeAction(); }

{whitespace}							{ return IgnoredLexemeAction(); }
.										{ return UnknownLexemeAction(); }

<<EOF>>									{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
