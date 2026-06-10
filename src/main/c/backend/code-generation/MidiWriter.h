#ifndef MIDI_WRITER_HEADER
#define MIDI_WRITER_HEADER

#include "MusicModel.h"
#include <stdbool.h>

/**
 * Writes a Standard MIDI File (SMF) from the given music composition.
 */
bool writeMidiFile(const MusicComposition * composition, const char * outputPath);

#endif
