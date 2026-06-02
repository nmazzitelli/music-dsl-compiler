#include "SemanticAnalyzer.h"
#include <string.h>

/* MODULE INTERNAL STATE */

typedef struct Symbol Symbol;

struct Symbol {
	const char * name;
	VarType type;
	Symbol * next;
};

static Logger * _logger = NULL;

/* PRIVATE FUNCTIONS */

static CompilationStatus _analyzeEvent(Event * event, Symbol ** symbols);
static CompilationStatus _analyzeEventList(EventList * events, Symbol ** symbols);
static CompilationStatus _analyzeExpression(Expression * expression, Symbol * symbols);
static CompilationStatus _analyzeGlobalSetting(GlobalSetting * setting);
static CompilationStatus _analyzeProgram(Program * program);
static CompilationStatus _analyzeTrack(Track * track);
static CompilationStatus _declareSymbol(Symbol ** symbols, const char * name, VarType type);
static void _destroySymbols(Symbol * symbols);
static Symbol * _findSymbol(Symbol * symbols, const char * name);
static CompilationStatus _validateChordPitches(ChordNoteList * notes);
static CompilationStatus _validatePitch(const char * pitch);
static CompilationStatus _validateUniqueTrackNames(TrackList * tracks);
static CompilationStatus _validateVelocity(Expression * velocity, Symbol * symbols);
static bool _isValidPitchAccidental(char accidental);
static bool _isValidPitchNoteClass(char noteClass);
static bool _isValidKeyNoteClass(const char * noteClass);
static bool _isValidTimeSignatureDenominator(int denominator);
static bool _isSupportedInstrument(const char * instrument);

static bool _isValidKeyNoteClass(const char * noteClass) {
	static const char * validNoteClasses[] = {
		"A",
		"B",
		"C",
		"D",
		"E",
		"F",
		"G"
	};
	for (size_t index = 0; index < sizeof(validNoteClasses) / sizeof(validNoteClasses[0]); ++index) {
		if (strcmp(noteClass, validNoteClasses[index]) == 0) {
			return true;
		}
	}
	return false;
}

static bool _isValidPitchNoteClass(char noteClass) {
	switch (noteClass) {
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
			return true;
		default:
			return false;
	}
}

static bool _isValidPitchAccidental(char accidental) {
	switch (accidental) {
		case '#':
		case 'b':
		case 'n':
			return true;
		default:
			return false;
	}
}

static bool _isValidTimeSignatureDenominator(int denominator) {
	switch (denominator) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
			return true;
		default:
			return false;
	}
}

static bool _isSupportedInstrument(const char * instrument) {
	static const char * supportedInstruments[] = {
		"piano",
		"guitar",
		"bass",
		"violin",
		"flute",
		"drums"
	};
	for (size_t index = 0; index < sizeof(supportedInstruments) / sizeof(supportedInstruments[0]); ++index) {
		if (strcmp(instrument, supportedInstruments[index]) == 0) {
			return true;
		}
	}
	return false;
}

static Symbol * _findSymbol(Symbol * symbols, const char * name) {
	for (Symbol * currentSymbol = symbols; currentSymbol != NULL; currentSymbol = currentSymbol->next) {
		if (strcmp(currentSymbol->name, name) == 0) {
			return currentSymbol;
		}
	}
	return NULL;
}

static CompilationStatus _declareSymbol(Symbol ** symbols, const char * name, VarType type) {
	Symbol * symbol = NULL;
	if (_findSymbol(*symbols, name) != NULL) {
		logError(_logger, "Variable \"%s\" is already declared in this track.", name);
		return FAILED;
	}
	symbol = calloc(1, sizeof(Symbol));
	symbol->name = name;
	symbol->type = type;
	symbol->next = *symbols;
	*symbols = symbol;
	return SUCCEEDED;
}

static void _destroySymbols(Symbol * symbols) {
	while (symbols != NULL) {
		Symbol * nextSymbol = symbols->next;
		free(symbols);
		symbols = nextSymbol;
	}
}

static CompilationStatus _validatePitch(const char * pitch) {
	size_t length = strlen(pitch);
	size_t octaveIndex = 0;
	int octave = 0;
	if (length != 2 && length != 3) {
		logError(_logger, "Invalid pitch \"%s\".", pitch);
		return FAILED;
	}
	if (!_isValidPitchNoteClass(pitch[0])) {
		logError(_logger, "Invalid pitch \"%s\".", pitch);
		return FAILED;
	}
	if (length == 3) {
		if (!_isValidPitchAccidental(pitch[1])) {
			logError(_logger, "Invalid pitch \"%s\".", pitch);
			return FAILED;
		}
		octaveIndex = 2;
	}
	else {
		octaveIndex = 1;
	}
	if (pitch[octaveIndex] < '0' || pitch[octaveIndex] > '9') {
		logError(_logger, "Invalid pitch \"%s\".", pitch);
		return FAILED;
	}
	octave = pitch[octaveIndex] - '0';
	if (octave < 0 || octave > 8) {
		logError(_logger, "Pitch \"%s\" is outside the supported octave range.", pitch);
		return FAILED;
	}
	return SUCCEEDED;
}

static CompilationStatus _analyzeExpression(Expression * expression, Symbol * symbols) {
	if (expression == NULL) {
		return SUCCEEDED;
	}
	switch (expression->type) {
		case EXPR_INTEGER:
		case EXPR_BOOLEAN:
		case EXPR_STRING:
			return SUCCEEDED;
		case EXPR_IDENTIFIER:
			if (_findSymbol(symbols, expression->stringValue) == NULL) {
				logError(_logger, "Variable \"%s\" is used before declaration.", expression->stringValue);
				return FAILED;
			}
			return SUCCEEDED;
		case EXPR_NOT:
			return _analyzeExpression(expression->operand, symbols);
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_LT:
		case EXPR_GT:
		case EXPR_EQ:
		case EXPR_NEQ:
		case EXPR_LEQ:
		case EXPR_GEQ:
		case EXPR_AND:
		case EXPR_OR: {
			CompilationStatus status = _analyzeExpression(expression->left, symbols);
			if (status != SUCCEEDED) {
				return status;
			}
			return _analyzeExpression(expression->right, symbols);
		}
		default:
			logError(_logger, "Unknown expression type %d.", expression->type);
			return FAILED;
	}
}

static CompilationStatus _validateChordPitches(ChordNoteList * notes) {
	for (ChordNoteList * currentNote = notes; currentNote != NULL; currentNote = currentNote->next) {
		CompilationStatus status = _validatePitch(currentNote->pitch);
		if (status != SUCCEEDED) {
			return status;
		}
	}
	return SUCCEEDED;
}

static CompilationStatus _validateVelocity(Expression * velocity, Symbol * symbols) {
	CompilationStatus status = _analyzeExpression(velocity, symbols);
	if (status != SUCCEEDED) {
		return status;
	}
	if (velocity == NULL) {
		return SUCCEEDED;
	}
	if (velocity->type != EXPR_INTEGER) {
		return SUCCEEDED;
	}
	if (velocity->intValue < 0 || velocity->intValue > 127) {
		logError(_logger, "Velocity %d is outside the MIDI range.", velocity->intValue);
		return FAILED;
	}
	return SUCCEEDED;
}

static CompilationStatus _analyzeEvent(Event * event, Symbol ** symbols) {
	switch (event->type) {
		case EVENT_NOTE: {
			CompilationStatus status = _validatePitch(event->note.pitch);
			if (status != SUCCEEDED) {
				return status;
			}
			return _validateVelocity(event->note.velocity, *symbols);
		}
		case EVENT_CHORD: {
			CompilationStatus status = _validateChordPitches(event->chord.notes);
			if (status != SUCCEEDED) {
				return status;
			}
			return _validateVelocity(event->chord.velocity, *symbols);
		}
		case EVENT_REPEAT:
			if (_analyzeExpression(event->repeat.count, *symbols) != SUCCEEDED) {
				return FAILED;
			}
			return _analyzeEventList(event->repeat.body, symbols);
		case EVENT_IF: {
			CompilationStatus status = _analyzeExpression(event->ifStatement.condition, *symbols);
			if (status != SUCCEEDED) {
				return status;
			}
			status = _analyzeEventList(event->ifStatement.thenBody, symbols);
			if (status != SUCCEEDED) {
				return status;
			}
			return _analyzeEventList(event->ifStatement.elseBody, symbols);
		}
		case EVENT_VAR_DECL: {
			CompilationStatus status = _analyzeExpression(event->varDecl.value, *symbols);
			if (status != SUCCEEDED) {
				return status;
			}
			return _declareSymbol(symbols, event->varDecl.name, event->varDecl.varType);
		}
		case EVENT_REST:
			return SUCCEEDED;
		default:
			logError(_logger, "Unknown event type %d.", event->type);
			return FAILED;
	}
}

static CompilationStatus _analyzeEventList(EventList * events, Symbol ** symbols) {
	CompilationStatus status = SUCCEEDED;
	if (events == NULL) {
		return SUCCEEDED;
	}
	status = _analyzeEventList(events->next, symbols);
	if (status != SUCCEEDED) {
		return status;
	}
	return _analyzeEvent(events->event, symbols);
}

static CompilationStatus _analyzeTrack(Track * track) {
	Symbol * symbols = NULL;
	CompilationStatus status = SUCCEEDED;
	logDebugging(_logger, "Visiting track \"%s\".", track->name);
	if (!_isSupportedInstrument(track->instrument)) {
		logError(_logger, "Unsupported instrument \"%s\" in track \"%s\".", track->instrument, track->name);
		return FAILED;
	}
	status = _analyzeEventList(track->events, &symbols);
	_destroySymbols(symbols);
	return status;
}

static CompilationStatus _analyzeGlobalSetting(GlobalSetting * setting) {
	switch (setting->type) {
		case SETTING_TEMPO:
			if (setting->tempo <= 0) {
				logError(_logger, "Tempo must be greater than zero.");
				return FAILED;
			}
			return SUCCEEDED;
		case SETTING_TIME_SIGNATURE:
			if (setting->timeSignature.numerator <= 0) {
				logError(_logger, "Time signature numerator must be greater than zero.");
				return FAILED;
			}
			if (setting->timeSignature.denominator <= 0) {
				logError(_logger, "Time signature denominator must be greater than zero.");
				return FAILED;
			}
			if (!_isValidTimeSignatureDenominator(setting->timeSignature.denominator)) {
				logError(_logger, "Unsupported time signature denominator %d.", setting->timeSignature.denominator);
				return FAILED;
			}
			return SUCCEEDED;
		case SETTING_KEY:
			if (!_isValidKeyNoteClass(setting->key.noteClass)) {
				logError(_logger, "Invalid key note class \"%s\".", setting->key.noteClass);
				return FAILED;
			}
			return SUCCEEDED;
		default:
			logError(_logger, "Unknown global setting type %d.", setting->type);
			return FAILED;
	}
}

static CompilationStatus _validateUniqueTrackNames(TrackList * tracks) {
	for (TrackList * currentTrack = tracks; currentTrack != NULL; currentTrack = currentTrack->next) {
		for (TrackList * nextTrack = currentTrack->next; nextTrack != NULL; nextTrack = nextTrack->next) {
			if (strcmp(currentTrack->track->name, nextTrack->track->name) == 0) {
				logError(_logger, "Duplicate track name \"%s\".", currentTrack->track->name);
				return FAILED;
			}
		}
	}
	return SUCCEEDED;
}

static CompilationStatus _analyzeProgram(Program * program) {
	logDebugging(_logger, "Analyzing program node...");
	for (GlobalSettingList * currentSetting = program->settings; currentSetting != NULL; currentSetting = currentSetting->next) {
		logDebugging(_logger, "Visiting global setting of type %d.", currentSetting->setting->type);
		CompilationStatus status = _analyzeGlobalSetting(currentSetting->setting);
		if (status != SUCCEEDED) {
			return status;
		}
	}
	CompilationStatus status = _validateUniqueTrackNames(program->tracks);
	if (status != SUCCEEDED) {
		return status;
	}
	for (TrackList * currentTrack = program->tracks; currentTrack != NULL; currentTrack = currentTrack->next) {
		status = _analyzeTrack(currentTrack->track);
		if (status != SUCCEEDED) {
			return status;
		}
	}
	return SUCCEEDED;
}

/* Cleans the module state. */
void _shutdownSemanticAnalyzerModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: SemanticAnalyzer...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeSemanticAnalyzerModule() {
	_logger = createLogger("SemanticAnalyzer");
	return _shutdownSemanticAnalyzerModule;
}

/** PUBLIC FUNCTIONS */

CompilationStatus executeSemanticAnalysis(CompilerState * compilerState) {
	logDebugging(_logger, "Executing semantic analysis...");
	if (compilerState == NULL) {
		logError(_logger, "Semantic analysis failed: compiler state is NULL.");
		return FAILED;
	}
	Program * program = compilerState->abstractSyntaxtTree;
	if (program == NULL) {
		logError(_logger, "Semantic analysis failed: AST root is NULL.");
		return FAILED;
	}
	CompilationStatus status = _analyzeProgram(program);
	logDebugging(_logger, "Semantic analysis finished with status %d.", status);
	return status;
}
