#include "SemanticAnalyzer.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/* PRIVATE FUNCTIONS */

static CompilationStatus _analyzeProgram(Program * program);

static CompilationStatus _analyzeProgram(Program * program) {
	logDebugging(_logger, "Analyzing program node...");
	for (GlobalSettingList * currentSetting = program->settings; currentSetting != NULL; currentSetting = currentSetting->next) {
		logDebugging(_logger, "Visiting global setting of type %d.", currentSetting->setting->type);
	}
	for (TrackList * currentTrack = program->tracks; currentTrack != NULL; currentTrack = currentTrack->next) {
		logDebugging(_logger, "Visiting track \"%s\".", currentTrack->track->name);
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
