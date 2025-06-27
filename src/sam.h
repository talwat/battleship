#ifndef SAM_H
#define SAM_H

#include <SDL.h>
#include <SDL_audio.h>

// Extern variables
extern int pos;
extern int debug;

// Function declarations
void MixAudio(void *unused, Uint8 *stream, int len);
void OutputSound(void);
void SetInput(char *_input);
void SetSpeed(unsigned char _speed);
void SetPitch(unsigned char _pitch);
void SetMouth(unsigned char _mouth);
void SetThroat(unsigned char _throat);
void EnableSingmode();
void EnableDebug();
int SAMMain();
char *GetBuffer();
int GetBufferLength();
int TextToPhonemes(unsigned char *input);

#endif // SAM_H