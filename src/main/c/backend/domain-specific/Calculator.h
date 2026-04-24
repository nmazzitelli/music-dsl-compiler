#ifndef CALCULATOR_HEADER
#define CALCULATOR_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

/** Initialize module's internal state. */
ModuleDestructor initializeCalculatorModule();

/** Executes the calculator using the current compiler state. */
void executeCalculator(CompilerState * compilerState);

#endif
