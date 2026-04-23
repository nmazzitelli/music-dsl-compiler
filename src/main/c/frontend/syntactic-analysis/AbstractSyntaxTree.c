#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

void destroyExpression(Expression * expression) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (expression != NULL) {
		switch (expression->type) {
			case EXPR_STRING:
			case EXPR_IDENTIFIER:
				free(expression->stringValue);
				break;
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
			case EXPR_OR:
				destroyExpression(expression->left);
				destroyExpression(expression->right);
				break;
			case EXPR_NOT:
				destroyExpression(expression->operand);
				break;
			case EXPR_INTEGER:
			case EXPR_BOOLEAN:
				break;
		}
		free(expression);
	}
}

void destroyChordNoteList(ChordNoteList * list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	while (list != NULL) {
		ChordNoteList * next = list->next;
		free(list->pitch);
		free(list);
		list = next;
	}
}

void destroyEvent(Event * event) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (event != NULL) {
		switch (event->type) {
			case EVENT_NOTE:
				free(event->note.pitch);
				destroyExpression(event->note.velocity);
				break;
			case EVENT_CHORD:
				destroyChordNoteList(event->chord.notes);
				destroyExpression(event->chord.velocity);
				break;
			case EVENT_REST:
				break;
			case EVENT_REPEAT:
				destroyExpression(event->repeat.count);
				destroyEventList(event->repeat.body);
				break;
			case EVENT_IF:
				destroyExpression(event->ifStatement.condition);
				destroyEventList(event->ifStatement.thenBody);
				destroyEventList(event->ifStatement.elseBody);
				break;
			case EVENT_VAR_DECL:
				free(event->varDecl.name);
				destroyExpression(event->varDecl.value);
				break;
		}
		free(event);
	}
}

void destroyEventList(EventList * list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	while (list != NULL) {
		EventList * next = list->next;
		destroyEvent(list->event);
		free(list);
		list = next;
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		free(program);
	}
}
