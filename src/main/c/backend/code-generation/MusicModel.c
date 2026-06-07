#include "MusicModel.h"

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
