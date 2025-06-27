#ifndef SAM_H
#define SAM_H

#include <SDL.h>
#include <SDL_audio.h>

extern int debug;

/**
 * Initializes SDL audio subsystem and opens an audio device for playback.
 * Call this function once before any audio playback to initialize SDL audio.
 *
 * @return 0 on success, -1 on failure.
 */
int InitSAMAudio();

/**
 * Converts the provided text into an audio buffer containing synthesized speech.
 *
 * @param text Input null-terminated string to synthesize.
 * @param dest Output pointer to the generated audio buffer.
 * @param len Output length of the generated audio buffer.
 *
 * @return int Returns 0 on success, 1 if phoneme conversion fails.
 */
int GenerateSAM(char *text, unsigned char **dest, int *len);

/**
 * Cleans up SDL audio subsystem and closes audio device.
 */
void CloseSAMAudio();

/**
 * Synthesizes speech from text and queues audio playback asynchronously.
 *
 * This function converts the input text into SAM audio and immediately queues
 * it to the SDL audio device. Playback happens in the background without blocking
 * the calling thread. The function returns as soon as the audio is queued.
 *
 * @param volume Integer from 0 to 128.
 * @param text Input null-terminated string to speak.
 */
void SpeakSAM(int volume, char *text);

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