#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * Self-referencing type forward declarations.
 */

typedef enum DurationType DurationType;
typedef enum EventType EventType;
typedef enum ExpressionType ExpressionType;
typedef enum ModeType ModeType;
typedef enum SettingType SettingType;
typedef enum VarType VarType;

typedef struct ChordNoteList ChordNoteList;
typedef struct Event Event;
typedef struct EventList EventList;
typedef struct Expression Expression;
typedef struct GlobalSetting GlobalSetting;
typedef struct GlobalSettingList GlobalSettingList;
typedef struct Program Program;
typedef struct Track Track;
typedef struct TrackList TrackList;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum DurationType {
	DURATION_WHOLE,
	DURATION_HALF,
	DURATION_QUARTER,
	DURATION_EIGHTH,
	DURATION_SIXTEENTH
};

enum ModeType {
	MODE_MAJOR,
	MODE_MINOR
};

enum VarType {
	VAR_INTEGER,
	VAR_BOOLEAN,
	VAR_STRING
};

enum ExpressionType {
	EXPR_INTEGER,
	EXPR_BOOLEAN,
	EXPR_STRING,
	EXPR_IDENTIFIER,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_LT,
	EXPR_GT,
	EXPR_EQ,
	EXPR_NEQ,
	EXPR_LEQ,
	EXPR_GEQ,
	EXPR_AND,
	EXPR_OR,
	EXPR_NOT
};

enum EventType {
	EVENT_NOTE,
	EVENT_CHORD,
	EVENT_REST,
	EVENT_REPEAT,
	EVENT_IF,
	EVENT_VAR_DECL
};

enum SettingType {
	SETTING_TEMPO,
	SETTING_TIME_SIGNATURE,
	SETTING_KEY
};

struct Expression {
	union {
		int intValue;
		bool boolValue;
		char * stringValue;
		struct {
			Expression * left;
			Expression * right;
		};
		Expression * operand;
	};
	ExpressionType type;
};

struct ChordNoteList {
	char * pitch;
	ChordNoteList * next;
};

struct EventList {
	Event * event;
	EventList * next;
};

struct Program {
	GlobalSettingList * settings;
	TrackList * tracks;
};

/**
 * Node recursive destructors.
 */

void destroyChordNoteList(ChordNoteList * list);
void destroyExpression(Expression * expression);
void destroyProgram(Program * program);

#endif
