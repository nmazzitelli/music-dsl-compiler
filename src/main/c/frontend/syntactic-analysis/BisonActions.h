#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState);

/**
 * Bison semantic actions.
 */

Program * ProgramSemanticAction(GlobalSettingList * settings, TrackList * tracks);

GlobalSettingList * AppendGlobalSettingSemanticAction(GlobalSettingList * list, GlobalSetting * setting);
GlobalSetting * TempoSettingSemanticAction(const int bpm);
GlobalSetting * TimeSignatureSettingSemanticAction(const int numerator, const int denominator);
GlobalSetting * KeySettingSemanticAction(char * noteClass, const ModeType mode);

TrackList * AppendTrackSemanticAction(TrackList * list, Track * track);
TrackList * SingleTrackSemanticAction(Track * track);
Track * TrackSemanticAction(char * name, char * instrument, EventList * events);

EventList * AppendEventSemanticAction(EventList * list, Event * event);
Event * PlayNoteSemanticAction(char * pitch, const DurationType duration, Expression * velocity);
Event * PlayChordSemanticAction(ChordNoteList * notes, const DurationType duration, Expression * velocity);
Event * RestEventSemanticAction(const DurationType duration);
Event * RepeatEventSemanticAction(Expression * count, EventList * body);
Event * IfEventSemanticAction(Expression * condition, EventList * thenBody, EventList * elseBody);
Event * VarDeclEventSemanticAction(const VarType varType, char * name, Expression * value);

ChordNoteList * AppendChordNoteSemanticAction(ChordNoteList * list, char * pitch);
ChordNoteList * SingleChordNoteSemanticAction(char * pitch);

Expression * IntegerExpressionSemanticAction(const int value);
Expression * BooleanExpressionSemanticAction(const bool value);
Expression * StringExpressionSemanticAction(char * value);
Expression * IdentifierExpressionSemanticAction(char * name);
Expression * BinaryExpressionSemanticAction(Expression * left, Expression * right, const ExpressionType type);
Expression * UnaryNotExpressionSemanticAction(Expression * operand);

#endif
