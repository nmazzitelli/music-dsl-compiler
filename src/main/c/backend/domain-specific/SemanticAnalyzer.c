#include "SemanticAnalyzer.h"
#include <string.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/* PRIVATE FUNCTIONS */

static CompilationStatus _analyzeGlobalSetting(GlobalSetting * setting);
static CompilationStatus _analyzeProgram(Program * program);
static CompilationStatus _analyzeTrack(Track * track);
static CompilationStatus _validateUniqueTrackNames(TrackList * tracks);
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

static CompilationStatus _analyzeTrack(Track * track) {
	logDebugging(_logger, "Visiting track \"%s\".", track->name);
	if (!_isSupportedInstrument(track->instrument)) {
		logError(_logger, "Unsupported instrument \"%s\" in track \"%s\".", track->instrument, track->name);
		return FAILED;
	}
	return SUCCEEDED;
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
