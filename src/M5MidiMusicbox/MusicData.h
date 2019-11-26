#ifndef __MUSIC_DATA_H__
#define __MUSIC_DATA_H__
#include "Arduino.h"

#define MAX_NOTE_COUNT   (2)

typedef struct {
  uint8_t size;
  uint8_t value[MAX_NOTE_COUNT];
  uint8_t length;
} NoteData_t;

typedef struct {
  const char *name;
  int length;
  const NoteData_t* data;
} MusicInfo_t;

#endif //__MUSIC_DATA_H__
