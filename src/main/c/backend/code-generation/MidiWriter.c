#include "MidiWriter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TICKS_PER_QUARTER 480

typedef struct ByteBuffer ByteBuffer;
typedef struct ScheduledEvent ScheduledEvent;

typedef enum ScheduledEventType {
	SCHEDULED_PROGRAM_CHANGE,
	SCHEDULED_NOTE_ON,
	SCHEDULED_NOTE_OFF
} ScheduledEventType;

struct ByteBuffer {
	uint8_t * data;
	size_t size;
	size_t capacity;
};

struct ScheduledEvent {
	int tick;
	ScheduledEventType type;
	int channel;
	int arg1;
	int arg2;
	ScheduledEvent * next;
};

static bool _appendBytes(ByteBuffer * buffer, const uint8_t * bytes, size_t length);
static bool _appendByte(ByteBuffer * buffer, uint8_t value);
static bool _appendVLQ(ByteBuffer * buffer, uint32_t value);
static void _destroyByteBuffer(ByteBuffer * buffer);
static void _destroyScheduledEvents(ScheduledEvent * events);
static int _countTracks(const MusicComposition * composition);
static int _getTimeSignatureDenominatorPower(int denominator);
static bool _insertScheduledEvent(ScheduledEvent ** head, ScheduledEvent * event);
static bool _scheduleTrackEvents(const MusicTrack * track, ScheduledEvent ** events);
static bool _buildTrackBuffer(const ScheduledEvent * events, int endTick, ByteBuffer * buffer);
static bool _buildConductorTrackBuffer(const MusicComposition * composition, ByteBuffer * buffer);
static bool _buildMusicTrackBuffer(const MusicTrack * track, ByteBuffer * buffer);
static bool _writeChunk(FILE * file, const char * type, const ByteBuffer * buffer);
static bool _writeHeader(FILE * file, uint16_t format, uint16_t trackCount, uint16_t division);

static bool _appendBytes(ByteBuffer * buffer, const uint8_t * bytes, size_t length) {
	size_t requiredSize = buffer->size + length;
	if (requiredSize > buffer->capacity) {
		size_t newCapacity = buffer->capacity == 0 ? 256 : buffer->capacity;
		while (newCapacity < requiredSize) {
			newCapacity *= 2;
		}
		uint8_t * resizedData = realloc(buffer->data, newCapacity);
		if (resizedData == NULL) {
			return false;
		}
		buffer->data = resizedData;
		buffer->capacity = newCapacity;
	}
	memcpy(buffer->data + buffer->size, bytes, length);
	buffer->size += length;
	return true;
}

static bool _appendByte(ByteBuffer * buffer, uint8_t value) {
	return _appendBytes(buffer, &value, 1);
}

static bool _appendVLQ(ByteBuffer * buffer, uint32_t value) {
	uint8_t bytes[4];
	size_t byteCount = 0;
	/* A standard MIDI VLQ holds at most 28 bits (four 7-bit groups); larger values would overflow bytes[4]. */
	if (value > 0x0FFFFFFF) {
		return false;
	}
	bytes[3] = (uint8_t) (value & 0x7F);
	byteCount = 1;
	value >>= 7;
	while (value > 0) {
		bytes[3 - byteCount] = (uint8_t) (0x80 | (value & 0x7F));
		byteCount += 1;
		value >>= 7;
	}
	return _appendBytes(buffer, bytes + (4 - byteCount), byteCount);
}

static void _destroyByteBuffer(ByteBuffer * buffer) {
	if (buffer != NULL) {
		free(buffer->data);
		buffer->data = NULL;
		buffer->size = 0;
		buffer->capacity = 0;
	}
}

static void _destroyScheduledEvents(ScheduledEvent * events) {
	while (events != NULL) {
		ScheduledEvent * next = events->next;
		free(events);
		events = next;
	}
}

static int _countTracks(const MusicComposition * composition) {
	int trackCount = 1;
	for (const MusicTrack * track = composition->tracks; track != NULL; track = track->next) {
		trackCount += 1;
	}
	return trackCount;
}

static int _getTimeSignatureDenominatorPower(int denominator) {
	int power = 0;
	int value = 1;
	while (value < denominator) {
		value *= 2;
		power += 1;
	}
	return power;
}

static bool _insertScheduledEvent(ScheduledEvent ** head, ScheduledEvent * event) {
	if (*head == NULL || event->tick < (*head)->tick) {
		event->next = *head;
		*head = event;
		return true;
	}
	ScheduledEvent * current = *head;
	while (current->next != NULL && current->next->tick <= event->tick) {
		current = current->next;
	}
	event->next = current->next;
	current->next = event;
	return true;
}

static bool _scheduleTrackEvents(const MusicTrack * track, ScheduledEvent ** events) {
	int currentTick = 0;
	ScheduledEvent * programChange = calloc(1, sizeof(ScheduledEvent));
	if (programChange == NULL) {
		return false;
	}
	programChange->tick = 0;
	programChange->type = SCHEDULED_PROGRAM_CHANGE;
	programChange->channel = track->channel;
	programChange->arg1 = track->instrumentProgram;
	if (!_insertScheduledEvent(events, programChange)) {
		free(programChange);
		return false;
	}
	for (const MusicEvent * event = track->events; event != NULL; event = event->next) {
		switch (event->type) {
			case MUSIC_EVENT_NOTE: {
				ScheduledEvent * noteOn = calloc(1, sizeof(ScheduledEvent));
				ScheduledEvent * noteOff = calloc(1, sizeof(ScheduledEvent));
				if (noteOn == NULL || noteOff == NULL) {
					free(noteOn);
					free(noteOff);
					return false;
				}
				noteOn->tick = currentTick;
				noteOn->type = SCHEDULED_NOTE_ON;
				noteOn->channel = track->channel;
				noteOn->arg1 = event->note.midiPitch;
				noteOn->arg2 = event->note.velocity;
				noteOff->tick = currentTick + event->note.durationTicks;
				noteOff->type = SCHEDULED_NOTE_OFF;
				noteOff->channel = track->channel;
				noteOff->arg1 = event->note.midiPitch;
				noteOff->arg2 = 0;
				if (!_insertScheduledEvent(events, noteOn) || !_insertScheduledEvent(events, noteOff)) {
					free(noteOn);
					free(noteOff);
					return false;
				}
				currentTick += event->note.durationTicks;
				break;
			}
			case MUSIC_EVENT_CHORD: {
				int durationTicks = event->chord.durationTicks;
				for (const MusicNoteList * note = event->chord.notes; note != NULL; note = note->next) {
					ScheduledEvent * noteOn = calloc(1, sizeof(ScheduledEvent));
					ScheduledEvent * noteOff = calloc(1, sizeof(ScheduledEvent));
					if (noteOn == NULL || noteOff == NULL) {
						free(noteOn);
						free(noteOff);
						return false;
					}
					noteOn->tick = currentTick;
					noteOn->type = SCHEDULED_NOTE_ON;
					noteOn->channel = track->channel;
					noteOn->arg1 = note->note.midiPitch;
					noteOn->arg2 = event->chord.velocity;
					noteOff->tick = currentTick + durationTicks;
					noteOff->type = SCHEDULED_NOTE_OFF;
					noteOff->channel = track->channel;
					noteOff->arg1 = note->note.midiPitch;
					noteOff->arg2 = 0;
					if (!_insertScheduledEvent(events, noteOn) || !_insertScheduledEvent(events, noteOff)) {
						free(noteOn);
						free(noteOff);
						return false;
					}
				}
				currentTick += durationTicks;
				break;
			}
			case MUSIC_EVENT_REST:
				currentTick += event->rest.durationTicks;
				break;
		}
	}
	return true;
}

static bool _buildTrackBuffer(const ScheduledEvent * events, int endTick, ByteBuffer * buffer) {
	int previousTick = 0;
	for (const ScheduledEvent * current = events; current != NULL; current = current->next) {
		uint32_t delta = (uint32_t) (current->tick - previousTick);
		if (!_appendVLQ(buffer, delta)) {
			return false;
		}
		switch (current->type) {
			case SCHEDULED_PROGRAM_CHANGE:
				if (!_appendByte(buffer, (uint8_t) (0xC0 | (current->channel & 0x0F))) ||
					!_appendByte(buffer, (uint8_t) (current->arg1 & 0x7F))) {
					return false;
				}
				break;
			case SCHEDULED_NOTE_ON:
				if (!_appendByte(buffer, (uint8_t) (0x90 | (current->channel & 0x0F))) ||
					!_appendByte(buffer, (uint8_t) (current->arg1 & 0x7F)) ||
					!_appendByte(buffer, (uint8_t) (current->arg2 & 0x7F))) {
					return false;
				}
				break;
			case SCHEDULED_NOTE_OFF:
				if (!_appendByte(buffer, (uint8_t) (0x80 | (current->channel & 0x0F))) ||
					!_appendByte(buffer, (uint8_t) (current->arg1 & 0x7F)) ||
					!_appendByte(buffer, (uint8_t) (current->arg2 & 0x7F))) {
					return false;
				}
				break;
		}
		previousTick = current->tick;
	}
	if (!_appendVLQ(buffer, (uint32_t) (endTick - previousTick)) ||
		!_appendByte(buffer, 0xFF) ||
		!_appendByte(buffer, 0x2F) ||
		!_appendByte(buffer, 0x00)) {
		return false;
	}
	return true;
}

static bool _buildConductorTrackBuffer(const MusicComposition * composition, ByteBuffer * buffer) {
	int tempo = composition->tempo > 0 ? composition->tempo : 120;
	uint32_t microsecondsPerQuarter = 60000000u / (uint32_t) tempo;
	uint8_t tempoBytes[3] = {
		(uint8_t) ((microsecondsPerQuarter >> 16) & 0xFF),
		(uint8_t) ((microsecondsPerQuarter >> 8) & 0xFF),
		(uint8_t) (microsecondsPerQuarter & 0xFF)
	};
	uint8_t timeSignatureBytes[4] = {
		(uint8_t) (composition->timeSignatureNumerator > 0 ? composition->timeSignatureNumerator : 4),
		(uint8_t) _getTimeSignatureDenominatorPower(
			composition->timeSignatureDenominator > 0 ? composition->timeSignatureDenominator : 4),
		24,
		8
	};
	if (!_appendVLQ(buffer, 0) ||
		!_appendByte(buffer, 0xFF) ||
		!_appendByte(buffer, 0x51) ||
		!_appendByte(buffer, 0x03) ||
		!_appendBytes(buffer, tempoBytes, sizeof(tempoBytes)) ||
		!_appendVLQ(buffer, 0) ||
		!_appendByte(buffer, 0xFF) ||
		!_appendByte(buffer, 0x58) ||
		!_appendByte(buffer, 0x04) ||
		!_appendBytes(buffer, timeSignatureBytes, sizeof(timeSignatureBytes)) ||
		!_appendVLQ(buffer, 0) ||
		!_appendByte(buffer, 0xFF) ||
		!_appendByte(buffer, 0x2F) ||
		!_appendByte(buffer, 0x00)) {
		return false;
	}
	return true;
}

static bool _buildMusicTrackBuffer(const MusicTrack * track, ByteBuffer * buffer) {
	ScheduledEvent * events = NULL;
	int endTick = 0;
	if (!_scheduleTrackEvents(track, &events)) {
		_destroyScheduledEvents(events);
		return false;
	}
	for (const MusicEvent * event = track->events; event != NULL; event = event->next) {
		switch (event->type) {
			case MUSIC_EVENT_NOTE:
				endTick += event->note.durationTicks;
				break;
			case MUSIC_EVENT_CHORD:
				endTick += event->chord.durationTicks;
				break;
			case MUSIC_EVENT_REST:
				endTick += event->rest.durationTicks;
				break;
		}
	}
	if (track->name != NULL && track->name[0] != '\0') {
		size_t nameLength = strlen(track->name);
		if (nameLength > 127) {
			nameLength = 127;
		}
		if (!_appendVLQ(buffer, 0) ||
			!_appendByte(buffer, 0xFF) ||
			!_appendByte(buffer, 0x03) ||
			!_appendByte(buffer, (uint8_t) nameLength) ||
			!_appendBytes(buffer, (const uint8_t *) track->name, nameLength)) {
			_destroyScheduledEvents(events);
			return false;
		}
	}
	if (!_buildTrackBuffer(events, endTick, buffer)) {
		_destroyScheduledEvents(events);
		return false;
	}
	_destroyScheduledEvents(events);
	return true;
}

static bool _writeChunk(FILE * file, const char * type, const ByteBuffer * buffer) {
	if (fwrite(type, 1, 4, file) != 4) {
		return false;
	}
	uint8_t lengthBytes[4] = {
		(uint8_t) ((buffer->size >> 24) & 0xFF),
		(uint8_t) ((buffer->size >> 16) & 0xFF),
		(uint8_t) ((buffer->size >> 8) & 0xFF),
		(uint8_t) (buffer->size & 0xFF)
	};
	if (fwrite(lengthBytes, 1, sizeof(lengthBytes), file) != sizeof(lengthBytes)) {
		return false;
	}
	if (buffer->size > 0 && fwrite(buffer->data, 1, buffer->size, file) != buffer->size) {
		return false;
	}
	return true;
}

static bool _writeHeader(FILE * file, uint16_t format, uint16_t trackCount, uint16_t division) {
	uint8_t headerChunk[14] = {
		'M', 'T', 'h', 'd',
		0, 0, 0, 6,
		(uint8_t) ((format >> 8) & 0xFF),
		(uint8_t) (format & 0xFF),
		(uint8_t) ((trackCount >> 8) & 0xFF),
		(uint8_t) (trackCount & 0xFF),
		(uint8_t) ((division >> 8) & 0xFF),
		(uint8_t) (division & 0xFF)
	};
	return fwrite(headerChunk, 1, sizeof(headerChunk), file) == sizeof(headerChunk);
}

/* PUBLIC FUNCTIONS */

bool writeMidiFile(const MusicComposition * composition, const char * outputPath) {
	FILE * file = NULL;
	ByteBuffer conductorTrack = {0};
	bool succeeded = false;
	if (composition == NULL || outputPath == NULL) {
		return false;
	}
	file = fopen(outputPath, "wb");
	if (file == NULL) {
		return false;
	}
	if (!_writeHeader(file, 1, (uint16_t) _countTracks(composition), TICKS_PER_QUARTER)) {
		goto cleanup;
	}
	if (!_buildConductorTrackBuffer(composition, &conductorTrack)) {
		goto cleanup;
	}
	if (!_writeChunk(file, "MTrk", &conductorTrack)) {
		goto cleanup;
	}
	for (const MusicTrack * track = composition->tracks; track != NULL; track = track->next) {
		ByteBuffer musicTrack = {0};
		if (!_buildMusicTrackBuffer(track, &musicTrack)) {
			_destroyByteBuffer(&musicTrack);
			goto cleanup;
		}
		if (!_writeChunk(file, "MTrk", &musicTrack)) {
			_destroyByteBuffer(&musicTrack);
			goto cleanup;
		}
		_destroyByteBuffer(&musicTrack);
	}
	succeeded = true;
cleanup:
	_destroyByteBuffer(&conductorTrack);
	if (file != NULL) {
		fclose(file);
	}
	if (!succeeded) {
		remove(outputPath);
	}
	return succeeded;
}
