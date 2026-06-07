#include "MusicModel.h"
#include <string.h>

static int _getPitchBaseSemitone(char noteClass);

static int _getPitchBaseSemitone(char noteClass) {
	switch (noteClass) {
		case 'C':
			return 0;
		case 'D':
			return 2;
		case 'E':
			return 4;
		case 'F':
			return 5;
		case 'G':
			return 7;
		case 'A':
			return 9;
		case 'B':
			return 11;
		default:
			return 0;
	}
}

int pitchToMidiNumber(const char * pitch, const MusicKey * key) {
	size_t length = strlen(pitch);
	int semitone = _getPitchBaseSemitone(pitch[0]);
	int octave = 0;
	(void) key;
	if (length == 3) {
		if (pitch[1] == '#') {
			semitone += 1;
		}
		else if (pitch[1] == 'b') {
			semitone -= 1;
		}
		octave = pitch[2] - '0';
	}
	else {
		octave = pitch[1] - '0';
	}
	return ((octave + 1) * 12) + semitone;
}

void destroyMusicNoteList(MusicNoteList * notes) {
	while (notes != NULL) {
		MusicNoteList * next = notes->next;
		free(notes);
		notes = next;
	}
}

void destroyMusicEventList(MusicEvent * events) {
	while (events != NULL) {
		MusicEvent * next = events->next;
		if (events->type == MUSIC_EVENT_CHORD) {
			destroyMusicNoteList(events->chord.notes);
		}
		free(events);
		events = next;
	}
}

void destroyMusicTrackList(MusicTrack * tracks) {
	while (tracks != NULL) {
		MusicTrack * next = tracks->next;
		free(tracks->name);
		destroyMusicEventList(tracks->events);
		free(tracks);
		tracks = next;
	}
}

void destroyMusicComposition(MusicComposition * composition) {
	if (composition != NULL) {
		free(composition->key.noteClass);
		destroyMusicTrackList(composition->tracks);
		free(composition);
	}
}
