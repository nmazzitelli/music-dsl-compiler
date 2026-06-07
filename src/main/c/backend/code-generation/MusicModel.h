#ifndef MUSIC_MODEL_HEADER
#define MUSIC_MODEL_HEADER

#include <stdbool.h>
#include <stdlib.h>

typedef enum MusicEventType MusicEventType;
typedef enum MusicKeyMode MusicKeyMode;

typedef struct MusicComposition MusicComposition;
typedef struct MusicEvent MusicEvent;
typedef struct MusicKey MusicKey;
typedef struct MusicNote MusicNote;
typedef struct MusicNoteList MusicNoteList;
typedef struct MusicTrack MusicTrack;

enum MusicEventType {
	MUSIC_EVENT_NOTE,
	MUSIC_EVENT_CHORD,
	MUSIC_EVENT_REST
};

enum MusicKeyMode {
	MUSIC_KEY_MAJOR,
	MUSIC_KEY_MINOR
};

struct MusicKey {
	bool isDefined;
	char * noteClass;
	MusicKeyMode mode;
};

struct MusicNote {
	int midiPitch;
	int durationTicks;
	int velocity;
};

struct MusicNoteList {
	MusicNote note;
	MusicNoteList * next;
};

struct MusicEvent {
	union {
		MusicNote note;
		struct {
			MusicNoteList * notes;
			int durationTicks;
			int velocity;
		} chord;
		struct {
			int durationTicks;
		} rest;
	};
	MusicEventType type;
	MusicEvent * next;
};

struct MusicTrack {
	char * name;
	int channel;
	int instrumentProgram;
	MusicEvent * events;
	MusicTrack * next;
};

struct MusicComposition {
	int tempo;
	int timeSignatureNumerator;
	int timeSignatureDenominator;
	MusicKey key;
	MusicTrack * tracks;
};

int pitchToMidiNumber(const char * pitch, const MusicKey * key);
void destroyMusicComposition(MusicComposition * composition);
void destroyMusicEventList(MusicEvent * events);
void destroyMusicNoteList(MusicNoteList * notes);
void destroyMusicTrackList(MusicTrack * tracks);

#endif
