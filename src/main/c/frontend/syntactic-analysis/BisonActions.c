#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* IMPORTED FUNCTIONS */

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

Program * ProgramSemanticAction(GlobalSettingList * settings, TrackList * tracks) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->settings = settings;
	program->tracks = tracks;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

GlobalSettingList * AppendGlobalSettingSemanticAction(GlobalSettingList * list, GlobalSetting * setting) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	GlobalSettingList * node = calloc(1, sizeof(GlobalSettingList));
	node->setting = setting;
	node->next = list;
	return node;
}

GlobalSetting * TempoSettingSemanticAction(const int bpm) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	GlobalSetting * setting = calloc(1, sizeof(GlobalSetting));
	setting->type = SETTING_TEMPO;
	setting->tempo = bpm;
	return setting;
}

GlobalSetting * TimeSignatureSettingSemanticAction(const int numerator, const int denominator) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	GlobalSetting * setting = calloc(1, sizeof(GlobalSetting));
	setting->type = SETTING_TIME_SIGNATURE;
	setting->timeSignature.numerator = numerator;
	setting->timeSignature.denominator = denominator;
	return setting;
}

GlobalSetting * KeySettingSemanticAction(char * noteClass, const ModeType mode) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	GlobalSetting * setting = calloc(1, sizeof(GlobalSetting));
	setting->type = SETTING_KEY;
	setting->key.noteClass = noteClass;
	setting->key.mode = mode;
	return setting;
}

TrackList * AppendTrackSemanticAction(TrackList * list, Track * track) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TrackList * node = calloc(1, sizeof(TrackList));
	node->track = track;
	node->next = list;
	return node;
}

TrackList * SingleTrackSemanticAction(Track * track) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TrackList * node = calloc(1, sizeof(TrackList));
	node->track = track;
	node->next = NULL;
	return node;
}

Track * TrackSemanticAction(char * name, char * instrument, EventList * events) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Track * track = calloc(1, sizeof(Track));
	track->name = name;
	track->instrument = instrument;
	track->events = events;
	return track;
}

EventList * AppendEventSemanticAction(EventList * list, Event * event) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	EventList * node = calloc(1, sizeof(EventList));
	node->event = event;
	node->next = list;
	return node;
}

Event * PlayNoteSemanticAction(char * pitch, const DurationType duration, Expression * velocity) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_NOTE;
	event->note.pitch = pitch;
	event->note.duration = duration;
	event->note.velocity = velocity;
	return event;
}

Event * PlayChordSemanticAction(ChordNoteList * notes, const DurationType duration, Expression * velocity) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_CHORD;
	event->chord.notes = notes;
	event->chord.duration = duration;
	event->chord.velocity = velocity;
	return event;
}

Event * RestEventSemanticAction(const DurationType duration) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_REST;
	event->rest.duration = duration;
	return event;
}

Event * RepeatEventSemanticAction(Expression * count, EventList * body) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_REPEAT;
	event->repeat.count = count;
	event->repeat.body = body;
	return event;
}

Event * IfEventSemanticAction(Expression * condition, EventList * thenBody, EventList * elseBody) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_IF;
	event->ifStatement.condition = condition;
	event->ifStatement.thenBody = thenBody;
	event->ifStatement.elseBody = elseBody;
	return event;
}

Event * VarDeclEventSemanticAction(const VarType varType, char * name, Expression * value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Event * event = calloc(1, sizeof(Event));
	event->type = EVENT_VAR_DECL;
	event->varDecl.varType = varType;
	event->varDecl.name = name;
	event->varDecl.value = value;
	return event;
}

ChordNoteList * AppendChordNoteSemanticAction(ChordNoteList * list, char * pitch) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ChordNoteList * node = calloc(1, sizeof(ChordNoteList));
	node->pitch = pitch;
	node->next = list;
	return node;
}

ChordNoteList * SingleChordNoteSemanticAction(char * pitch) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ChordNoteList * node = calloc(1, sizeof(ChordNoteList));
	node->pitch = pitch;
	node->next = NULL;
	return node;
}

Expression * IntegerExpressionSemanticAction(const int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = EXPR_INTEGER;
	expression->intValue = value;
	return expression;
}

Expression * BooleanExpressionSemanticAction(const bool value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = EXPR_BOOLEAN;
	expression->boolValue = value;
	return expression;
}

Expression * StringExpressionSemanticAction(char * value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = EXPR_STRING;
	expression->stringValue = value;
	return expression;
}

Expression * IdentifierExpressionSemanticAction(char * name) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = EXPR_IDENTIFIER;
	expression->stringValue = name;
	return expression;
}

Expression * BinaryExpressionSemanticAction(Expression * left, Expression * right, const ExpressionType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = type;
	expression->left = left;
	expression->right = right;
	return expression;
}

Expression * UnaryNotExpressionSemanticAction(Expression * operand) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->type = EXPR_NOT;
	expression->operand = operand;
	return expression;
}
