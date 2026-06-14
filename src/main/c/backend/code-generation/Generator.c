#include "Generator.h"
#include "MidiWriter.h"
#include <string.h>

/* MODULE INTERNAL STATE */

typedef enum RuntimeValueType RuntimeValueType;
typedef struct RuntimeValue RuntimeValue;
typedef struct RuntimeSymbol RuntimeSymbol;
typedef struct LoweringState LoweringState;

enum RuntimeValueType {
	RUNTIME_INTEGER,
	RUNTIME_BOOLEAN,
	RUNTIME_STRING
};

struct RuntimeValue {
	RuntimeValueType type;
	union {
		int intValue;
		bool boolValue;
		const char * stringValue;
	};
};

struct RuntimeSymbol {
	const char * name;
	RuntimeValue value;
	RuntimeSymbol * next;
};

struct LoweringState {
	RuntimeSymbol * symbols;
	MusicKey * key;
};

static Logger * _logger = NULL;

/* PRIVATE FUNCTIONS */

static void _appendMusicEvent(MusicEvent ** head, MusicEvent ** tail, MusicEvent * event);
static void _appendMusicTrack(MusicTrack ** head, MusicTrack ** tail, MusicTrack * track);
static CompilationStatus _copyMusicEventList(MusicEvent * source, MusicEvent ** copyHead, MusicEvent ** copyTail);
static CompilationStatus _copyMusicNoteList(MusicNoteList * source, MusicNoteList ** copyHead, MusicNoteList ** copyTail);
static CompilationStatus _createChordEvent(Event * event, LoweringState * state, MusicEvent ** musicEvent);
static MusicEvent * _createRestEvent(DurationType duration);
static CompilationStatus _createNoteEvent(Event * event, LoweringState * state, MusicEvent ** musicEvent);
static void _destroyRuntimeSymbols(RuntimeSymbol * symbols);
static CompilationStatus _evaluateBoolean(Expression * expression, RuntimeSymbol * symbols, bool * result);
static CompilationStatus _evaluateExpression(Expression * expression, RuntimeSymbol * symbols, RuntimeValue * result);
static CompilationStatus _evaluateInteger(Expression * expression, RuntimeSymbol * symbols, int * result);
static RuntimeSymbol * _findRuntimeSymbol(RuntimeSymbol * symbols, const char * name);
static int _getDefaultVelocity();
static int _getDurationTicks(DurationType duration);
static int _getInstrumentProgram(const char * instrument);
static int _getNextMelodicChannel(int currentChannel);
static CompilationStatus _lowerConditional(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail);
static CompilationStatus _lowerEvent(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail);
static CompilationStatus _lowerEventList(EventList * events, LoweringState * state, MusicEvent ** head, MusicEvent ** tail);
static CompilationStatus _lowerProgram(Program * program, MusicComposition ** composition);
static CompilationStatus _lowerRepeat(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail);
static CompilationStatus _lowerTrack(Track * track, MusicKey * key, int channel, MusicTrack ** musicTrack);
static MusicKeyMode _musicKeyModeFromAst(ModeType mode);
static MusicEvent * _newMusicEvent(MusicEventType type);
static CompilationStatus _populateCompositionSettings(Program * program, MusicComposition * composition);
static CompilationStatus _setRuntimeSymbol(RuntimeSymbol ** symbols, const char * name, RuntimeValue value);
static TrackList * _reverseTrackList(TrackList * list);

static void _appendMusicEvent(MusicEvent ** head, MusicEvent ** tail, MusicEvent * event) {
	if (*head == NULL) {
		*head = event;
		*tail = event;
	}
	else {
		(*tail)->next = event;
		*tail = event;
	}
}

static void _appendMusicTrack(MusicTrack ** head, MusicTrack ** tail, MusicTrack * track) {
	if (*head == NULL) {
		*head = track;
		*tail = track;
	}
	else {
		(*tail)->next = track;
		*tail = track;
	}
}

static CompilationStatus _copyMusicNoteList(MusicNoteList * source, MusicNoteList ** copyHead, MusicNoteList ** copyTail) {
	for (MusicNoteList * currentNote = source; currentNote != NULL; currentNote = currentNote->next) {
		MusicNoteList * copy = calloc(1, sizeof(MusicNoteList));
		if (copy == NULL) {
			destroyMusicNoteList(*copyHead);
			*copyHead = NULL;
			*copyTail = NULL;
			return OUT_OF_MEMORY;
		}
		copy->note = currentNote->note;
		if (*copyHead == NULL) {
			*copyHead = copy;
			*copyTail = copy;
		}
		else {
			(*copyTail)->next = copy;
			*copyTail = copy;
		}
	}
	return SUCCEEDED;
}

static CompilationStatus _copyMusicEventList(MusicEvent * source, MusicEvent ** copyHead, MusicEvent ** copyTail) {
	for (MusicEvent * currentEvent = source; currentEvent != NULL; currentEvent = currentEvent->next) {
		MusicEvent * copy = _newMusicEvent(currentEvent->type);
		if (copy == NULL) {
			destroyMusicEventList(*copyHead);
			*copyHead = NULL;
			*copyTail = NULL;
			return OUT_OF_MEMORY;
		}
		switch (currentEvent->type) {
			case MUSIC_EVENT_NOTE:
				copy->note = currentEvent->note;
				break;
			case MUSIC_EVENT_CHORD: {
				MusicNoteList * noteHead = NULL;
				MusicNoteList * noteTail = NULL;
				CompilationStatus status = _copyMusicNoteList(currentEvent->chord.notes, &noteHead, &noteTail);
				if (status != SUCCEEDED) {
					free(copy);
					return status;
				}
				copy->chord.notes = noteHead;
				copy->chord.durationTicks = currentEvent->chord.durationTicks;
				copy->chord.velocity = currentEvent->chord.velocity;
				break;
			}
			case MUSIC_EVENT_REST:
				copy->rest.durationTicks = currentEvent->rest.durationTicks;
				break;
		}
		_appendMusicEvent(copyHead, copyTail, copy);
	}
	return SUCCEEDED;
}

static CompilationStatus _createChordEvent(Event * event, LoweringState * state, MusicEvent ** musicEvent) {
	MusicEvent * loweredEvent = _newMusicEvent(MUSIC_EVENT_CHORD);
	MusicNoteList * head = NULL;
	MusicNoteList * tail = NULL;
	int velocity = _getDefaultVelocity();
	if (loweredEvent == NULL) {
		return OUT_OF_MEMORY;
	}
	if (event->chord.velocity != NULL) {
		CompilationStatus status = _evaluateInteger(event->chord.velocity, state->symbols, &velocity);
		if (status != SUCCEEDED) {
			free(loweredEvent);
			return status;
		}
	}
	for (ChordNoteList * currentNote = event->chord.notes; currentNote != NULL; currentNote = currentNote->next) {
		MusicNoteList * loweredNote = calloc(1, sizeof(MusicNoteList));
		if (loweredNote == NULL) {
			destroyMusicNoteList(head);
			free(loweredEvent);
			return OUT_OF_MEMORY;
		}
		loweredNote->note.midiPitch = pitchToMidiNumber(currentNote->pitch, state->key);
		loweredNote->note.durationTicks = _getDurationTicks(event->chord.duration);
		loweredNote->note.velocity = velocity;
		if (head == NULL) {
			head = loweredNote;
			tail = loweredNote;
		}
		else {
			tail->next = loweredNote;
			tail = loweredNote;
		}
	}
	loweredEvent->chord.notes = head;
	loweredEvent->chord.durationTicks = _getDurationTicks(event->chord.duration);
	loweredEvent->chord.velocity = velocity;
	*musicEvent = loweredEvent;
	return SUCCEEDED;
}

static MusicEvent * _createRestEvent(DurationType duration) {
	MusicEvent * event = _newMusicEvent(MUSIC_EVENT_REST);
	if (event != NULL) {
		event->rest.durationTicks = _getDurationTicks(duration);
	}
	return event;
}

static CompilationStatus _createNoteEvent(Event * event, LoweringState * state, MusicEvent ** musicEvent) {
	MusicEvent * loweredEvent = _newMusicEvent(MUSIC_EVENT_NOTE);
	int velocity = _getDefaultVelocity();
	if (loweredEvent == NULL) {
		return OUT_OF_MEMORY;
	}
	loweredEvent->note.midiPitch = pitchToMidiNumber(event->note.pitch, state->key);
	if (event->note.velocity != NULL) {
		CompilationStatus status = _evaluateInteger(event->note.velocity, state->symbols, &velocity);
		if (status != SUCCEEDED) {
			free(loweredEvent);
			return status;
		}
	}
	loweredEvent->note.durationTicks = _getDurationTicks(event->note.duration);
	loweredEvent->note.velocity = velocity;
	*musicEvent = loweredEvent;
	return SUCCEEDED;
}

static void _destroyRuntimeSymbols(RuntimeSymbol * symbols) {
	while (symbols != NULL) {
		RuntimeSymbol * next = symbols->next;
		free(symbols);
		symbols = next;
	}
}

static CompilationStatus _evaluateBoolean(Expression * expression, RuntimeSymbol * symbols, bool * result) {
	RuntimeValue value = {0};
	CompilationStatus status = _evaluateExpression(expression, symbols, &value);
	if (status != SUCCEEDED) {
		return status;
	}
	if (value.type != RUNTIME_BOOLEAN) {
		logError(_logger, "Expected a boolean expression while lowering.");
		return FAILED;
	}
	*result = value.boolValue;
	return SUCCEEDED;
}

static CompilationStatus _evaluateExpression(Expression * expression, RuntimeSymbol * symbols, RuntimeValue * result) {
	RuntimeValue leftValue = {0};
	RuntimeValue rightValue = {0};
	RuntimeSymbol * symbol = NULL;
	if (expression == NULL) {
		return FAILED;
	}
	switch (expression->type) {
		case EXPR_INTEGER:
			result->type = RUNTIME_INTEGER;
			result->intValue = expression->intValue;
			return SUCCEEDED;
		case EXPR_BOOLEAN:
			result->type = RUNTIME_BOOLEAN;
			result->boolValue = expression->boolValue;
			return SUCCEEDED;
		case EXPR_STRING:
			result->type = RUNTIME_STRING;
			result->stringValue = expression->stringValue;
			return SUCCEEDED;
		case EXPR_IDENTIFIER:
			symbol = _findRuntimeSymbol(symbols, expression->stringValue);
			if (symbol == NULL) {
				logError(_logger, "Variable \"%s\" is not available while lowering.", expression->stringValue);
				return FAILED;
			}
			*result = symbol->value;
			return SUCCEEDED;
		case EXPR_NOT:
			if (_evaluateExpression(expression->operand, symbols, &leftValue) != SUCCEEDED) {
				return FAILED;
			}
			result->type = RUNTIME_BOOLEAN;
			result->boolValue = !leftValue.boolValue;
			return SUCCEEDED;
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
			if (_evaluateExpression(expression->left, symbols, &leftValue) != SUCCEEDED
			 || _evaluateExpression(expression->right, symbols, &rightValue) != SUCCEEDED) {
				return FAILED;
			}
			result->type = RUNTIME_INTEGER;
			switch (expression->type) {
				case EXPR_ADD:
					result->intValue = leftValue.intValue + rightValue.intValue;
					break;
				case EXPR_SUB:
					result->intValue = leftValue.intValue - rightValue.intValue;
					break;
				case EXPR_MUL:
					result->intValue = leftValue.intValue * rightValue.intValue;
					break;
				case EXPR_DIV:
					if (rightValue.intValue == 0) {
						logError(_logger, "Integer division by zero while lowering.");
						return FAILED;
					}
					result->intValue = leftValue.intValue / rightValue.intValue;
					break;
				default:
					break;
			}
			return SUCCEEDED;
		case EXPR_LT:
		case EXPR_GT:
		case EXPR_LEQ:
		case EXPR_GEQ:
			if (_evaluateExpression(expression->left, symbols, &leftValue) != SUCCEEDED
			 || _evaluateExpression(expression->right, symbols, &rightValue) != SUCCEEDED) {
				return FAILED;
			}
			result->type = RUNTIME_BOOLEAN;
			switch (expression->type) {
				case EXPR_LT:
					result->boolValue = leftValue.intValue < rightValue.intValue;
					break;
				case EXPR_GT:
					result->boolValue = leftValue.intValue > rightValue.intValue;
					break;
				case EXPR_LEQ:
					result->boolValue = leftValue.intValue <= rightValue.intValue;
					break;
				case EXPR_GEQ:
					result->boolValue = leftValue.intValue >= rightValue.intValue;
					break;
				default:
					break;
			}
			return SUCCEEDED;
		case EXPR_EQ:
		case EXPR_NEQ:
			if (_evaluateExpression(expression->left, symbols, &leftValue) != SUCCEEDED
			 || _evaluateExpression(expression->right, symbols, &rightValue) != SUCCEEDED) {
				return FAILED;
			}
			result->type = RUNTIME_BOOLEAN;
			if (leftValue.type == RUNTIME_INTEGER) {
				result->boolValue = leftValue.intValue == rightValue.intValue;
			}
			else if (leftValue.type == RUNTIME_BOOLEAN) {
				result->boolValue = leftValue.boolValue == rightValue.boolValue;
			}
			else {
				result->boolValue = strcmp(leftValue.stringValue, rightValue.stringValue) == 0;
			}
			if (expression->type == EXPR_NEQ) {
				result->boolValue = !result->boolValue;
			}
			return SUCCEEDED;
		case EXPR_AND:
		case EXPR_OR:
			if (_evaluateExpression(expression->left, symbols, &leftValue) != SUCCEEDED
			 || _evaluateExpression(expression->right, symbols, &rightValue) != SUCCEEDED) {
				return FAILED;
			}
			result->type = RUNTIME_BOOLEAN;
			if (expression->type == EXPR_AND) {
				result->boolValue = leftValue.boolValue && rightValue.boolValue;
			}
			else {
				result->boolValue = leftValue.boolValue || rightValue.boolValue;
			}
			return SUCCEEDED;
	}
	return FAILED;
}

static CompilationStatus _evaluateInteger(Expression * expression, RuntimeSymbol * symbols, int * result) {
	RuntimeValue value = {0};
	CompilationStatus status = _evaluateExpression(expression, symbols, &value);
	if (status != SUCCEEDED) {
		return status;
	}
	if (value.type != RUNTIME_INTEGER) {
		logError(_logger, "Expected an integer expression while lowering.");
		return FAILED;
	}
	*result = value.intValue;
	return SUCCEEDED;
}

static RuntimeSymbol * _findRuntimeSymbol(RuntimeSymbol * symbols, const char * name) {
	for (RuntimeSymbol * currentSymbol = symbols; currentSymbol != NULL; currentSymbol = currentSymbol->next) {
		if (strcmp(currentSymbol->name, name) == 0) {
			return currentSymbol;
		}
	}
	return NULL;
}

static int _getDefaultVelocity() {
	return 64;
}

static int _getDurationTicks(DurationType duration) {
	switch (duration) {
		case DURATION_WHOLE:
			return 1920;
		case DURATION_HALF:
			return 960;
		case DURATION_QUARTER:
			return 480;
		case DURATION_EIGHTH:
			return 240;
		case DURATION_SIXTEENTH:
			return 120;
	}
	return 480;
}

static int _getInstrumentProgram(const char * instrument) {
	if (strcmp(instrument, "piano") == 0) {
		return 0;
	}
	if (strcmp(instrument, "guitar") == 0) {
		return 24;
	}
	if (strcmp(instrument, "bass") == 0) {
		return 32;
	}
	if (strcmp(instrument, "violin") == 0) {
		return 40;
	}
	if (strcmp(instrument, "flute") == 0) {
		return 73;
	}
	if (strcmp(instrument, "drums") == 0) {
		return 0;
	}
	return 0;
}

static int _getNextMelodicChannel(int currentChannel) {
	int nextChannel = currentChannel + 1;
	if (nextChannel == 9) {
		nextChannel += 1;
	}
	return nextChannel;
}

static CompilationStatus _lowerConditional(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail) {
	bool condition = false;
	CompilationStatus status = _evaluateBoolean(event->ifStatement.condition, state->symbols, &condition);
	if (status != SUCCEEDED) {
		return status;
	}
	if (condition) {
		return _lowerEventList(event->ifStatement.thenBody, state, head, tail);
	}
	return _lowerEventList(event->ifStatement.elseBody, state, head, tail);
}

static CompilationStatus _lowerEvent(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail) {
	MusicEvent * loweredEvent = NULL;
	RuntimeValue value = {0};
	switch (event->type) {
		case EVENT_NOTE:
			if (_createNoteEvent(event, state, &loweredEvent) != SUCCEEDED) {
				return FAILED;
			}
			_appendMusicEvent(head, tail, loweredEvent);
			return SUCCEEDED;
		case EVENT_CHORD:
			if (_createChordEvent(event, state, &loweredEvent) != SUCCEEDED) {
				return FAILED;
			}
			_appendMusicEvent(head, tail, loweredEvent);
			return SUCCEEDED;
		case EVENT_REST:
			loweredEvent = _createRestEvent(event->rest.duration);
			if (loweredEvent == NULL) {
				return OUT_OF_MEMORY;
			}
			_appendMusicEvent(head, tail, loweredEvent);
			return SUCCEEDED;
		case EVENT_REPEAT:
			return _lowerRepeat(event, state, head, tail);
		case EVENT_IF:
			return _lowerConditional(event, state, head, tail);
		case EVENT_VAR_DECL:
			if (_evaluateExpression(event->varDecl.value, state->symbols, &value) != SUCCEEDED) {
				return FAILED;
			}
			return _setRuntimeSymbol(&state->symbols, event->varDecl.name, value);
	}
	return FAILED;
}

static CompilationStatus _lowerEventList(EventList * events, LoweringState * state, MusicEvent ** head, MusicEvent ** tail) {
	CompilationStatus status = SUCCEEDED;
	if (events == NULL) {
		return SUCCEEDED;
	}
	status = _lowerEventList(events->next, state, head, tail);
	if (status != SUCCEEDED) {
		return status;
	}
	return _lowerEvent(events->event, state, head, tail);
}

static TrackList * _reverseTrackList(TrackList * list) {
	TrackList * prev = NULL;
	TrackList * current = list;
	while (current != NULL) {
		TrackList * next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	return prev;
}

static CompilationStatus _lowerProgram(Program * program, MusicComposition ** composition) {
	MusicComposition * loweredComposition = calloc(1, sizeof(MusicComposition));
	MusicTrack * trackHead = NULL;
	MusicTrack * trackTail = NULL;
	int melodicChannel = 0;
	if (loweredComposition == NULL) {
		return OUT_OF_MEMORY;
	}
	loweredComposition->tempo = 120;
	loweredComposition->timeSignatureNumerator = 4;
	loweredComposition->timeSignatureDenominator = 4;
	if (_populateCompositionSettings(program, loweredComposition) != SUCCEEDED) {
		destroyMusicComposition(loweredComposition);
		return FAILED;
	}
	program->tracks = _reverseTrackList(program->tracks);
	for (TrackList * currentTrack = program->tracks; currentTrack != NULL; currentTrack = currentTrack->next) {
		MusicTrack * loweredTrack = NULL;
		int channel = melodicChannel;
		if (strcmp(currentTrack->track->instrument, "drums") == 0) {
			channel = 9;
		}
		else {
			melodicChannel = _getNextMelodicChannel(melodicChannel);
		}
		if (_lowerTrack(currentTrack->track, &loweredComposition->key, channel, &loweredTrack) != SUCCEEDED) {
			destroyMusicComposition(loweredComposition);
			return FAILED;
		}
		_appendMusicTrack(&trackHead, &trackTail, loweredTrack);
	}
	loweredComposition->tracks = trackHead;
	*composition = loweredComposition;
	return SUCCEEDED;
}

static CompilationStatus _lowerRepeat(Event * event, LoweringState * state, MusicEvent ** head, MusicEvent ** tail) {
	int repeatCount = 0;
	MusicEvent * bodyHead = NULL;
	MusicEvent * bodyTail = NULL;
	CompilationStatus status = _evaluateInteger(event->repeat.count, state->symbols, &repeatCount);
	if (status != SUCCEEDED) {
		return status;
	}
	status = _lowerEventList(event->repeat.body, state, &bodyHead, &bodyTail);
	if (status != SUCCEEDED) {
		destroyMusicEventList(bodyHead);
		return status;
	}
	for (int repeatIndex = 0; repeatIndex < repeatCount; ++repeatIndex) {
		MusicEvent * iterationHead = NULL;
		MusicEvent * iterationTail = NULL;
		status = _copyMusicEventList(bodyHead, &iterationHead, &iterationTail);
		if (status != SUCCEEDED) {
			destroyMusicEventList(bodyHead);
			return status;
		}
		if (*head == NULL) {
			*head = iterationHead;
			*tail = iterationTail;
		}
		else {
			(*tail)->next = iterationHead;
			if (iterationTail != NULL) {
				*tail = iterationTail;
			}
		}
	}
	destroyMusicEventList(bodyHead);
	return SUCCEEDED;
}

static CompilationStatus _lowerTrack(Track * track, MusicKey * key, int channel, MusicTrack ** musicTrack) {
	LoweringState state = {.symbols = NULL, .key = key};
	MusicTrack * loweredTrack = calloc(1, sizeof(MusicTrack));
	MusicEvent * eventHead = NULL;
	MusicEvent * eventTail = NULL;
	if (loweredTrack == NULL) {
		return OUT_OF_MEMORY;
	}
	loweredTrack->name = strdup(track->name);
	if (loweredTrack->name == NULL) {
		free(loweredTrack);
		return OUT_OF_MEMORY;
	}
	loweredTrack->channel = channel;
	loweredTrack->instrumentProgram = _getInstrumentProgram(track->instrument);
	if (_lowerEventList(track->events, &state, &eventHead, &eventTail) != SUCCEEDED) {
		free(loweredTrack->name);
		free(loweredTrack);
		destroyMusicEventList(eventHead);
		_destroyRuntimeSymbols(state.symbols);
		return FAILED;
	}
	loweredTrack->events = eventHead;
	_destroyRuntimeSymbols(state.symbols);
	*musicTrack = loweredTrack;
	return SUCCEEDED;
}

static MusicKeyMode _musicKeyModeFromAst(ModeType mode) {
	return mode == MODE_MINOR ? MUSIC_KEY_MINOR : MUSIC_KEY_MAJOR;
}

static MusicEvent * _newMusicEvent(MusicEventType type) {
	MusicEvent * event = calloc(1, sizeof(MusicEvent));
	if (event != NULL) {
		event->type = type;
	}
	return event;
}

static CompilationStatus _populateCompositionSettings(Program * program, MusicComposition * composition) {
	for (GlobalSettingList * currentSetting = program->settings; currentSetting != NULL; currentSetting = currentSetting->next) {
		switch (currentSetting->setting->type) {
			case SETTING_TEMPO:
				composition->tempo = currentSetting->setting->tempo;
				break;
			case SETTING_TIME_SIGNATURE:
				composition->timeSignatureNumerator = currentSetting->setting->timeSignature.numerator;
				composition->timeSignatureDenominator = currentSetting->setting->timeSignature.denominator;
				break;
			case SETTING_KEY:
				composition->key.isDefined = true;
				free(composition->key.noteClass);
				composition->key.noteClass = strdup(currentSetting->setting->key.noteClass);
				if (composition->key.noteClass == NULL) {
					return OUT_OF_MEMORY;
				}
				composition->key.mode = _musicKeyModeFromAst(currentSetting->setting->key.mode);
				break;
		}
	}
	return SUCCEEDED;
}

static CompilationStatus _setRuntimeSymbol(RuntimeSymbol ** symbols, const char * name, RuntimeValue value) {
	RuntimeSymbol * symbol = _findRuntimeSymbol(*symbols, name);
	if (symbol != NULL) {
		symbol->value = value;
		return SUCCEEDED;
	}
	symbol = calloc(1, sizeof(RuntimeSymbol));
	if (symbol == NULL) {
		return OUT_OF_MEMORY;
	}
	symbol->name = name;
	symbol->value = value;
	symbol->next = *symbols;
	*symbols = symbol;
	return SUCCEEDED;
}

/** Shutdown module's internal state. */
void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Generator...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/* PUBLIC FUNCTIONS */

CompilationStatus executeGenerator(CompilerState * compilerState) {
	Program * program = NULL;
	logDebugging(_logger, "Generating final output...");
	if (compilerState == NULL) {
		logError(_logger, "Generator failed: compiler state is NULL.");
		return FAILED;
	}
	program = compilerState->abstractSyntaxtTree;
	if (program == NULL) {
		logError(_logger, "Generator failed: AST root is NULL.");
		return FAILED;
	}
	destroyMusicComposition(compilerState->musicComposition);
	compilerState->musicComposition = NULL;
	if (_lowerProgram(program, &compilerState->musicComposition) != SUCCEEDED) {
		logError(_logger, "Generator failed while lowering the AST.");
		return FAILED;
	}
	if (!writeMidiFile(compilerState->musicComposition, "output.mid")) {
		logError(_logger, "Generator failed while writing the MIDI file.");
		return FAILED;
	}
	logDebugging(_logger, "Generation is done.");
	return SUCCEEDED;
}
