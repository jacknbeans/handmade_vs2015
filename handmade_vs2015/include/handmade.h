#pragma once

#include <cstdint>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define ArrayCount(a_Array) (sizeof(a_Array) / sizeof((a_Array)[0]))

#define Kilobytes(a_Value) ((a_Value) * 1024)
#define Megabytes(a_Value) (Kilobytes(a_Value) * 1024)
#define Gigabytes(a_Value) (Megabytes(a_Value) * 1024)
#define Terabytes(a_Value) (Gigabytes(a_Value) * 1024)

/*
* HANDMADE_INTERNAL:
*     0 - Build for public release
*     1 - Build for developer only
*
* HANDMADE_SLOW:
*     0 - No slow code allowed!
*     1 - Slow code welcome.
*/

#if HANDMADE_SLOW
#define Assert(a_Expression) if (!(a_Expression)) { *(int *)0 = 0;}
#else
#define Assert(a_Expression)
#endif

#if HANDMADE_INTERNAL
/*
 * IMPORTANT: Not shipping code!
 * 
 * These are NOT for doing anything in the shipping game - they are blocking
 * and the write doesn't protect against lost data!
 */
struct DebugReadFileResult {
	uint32 contentsSize;
	void *contents;
};

extern DebugReadFileResult DEBUGPlatformReadEntireFile(char *a_Filename);
extern void DEBUGPlatformFreeFileMemory(void *a_Memory);
/**
 * \param a_Filename Cannot contain a non-existing path.
 */
extern bool32 DEBUGPlatformWriteEntireFile(char *a_Filename, uint32 a_MemorySize, void *a_Memory);
#endif

inline uint32 SafeTruncateUInt64(uint64 a_Value) {
	Assert(a_Value <= 0xFFFFFFFF);
	auto result = uint32(a_Value);
	return result;
}

struct GameOffscreenBuffer {
	void *memory;
	int width;
	int height;
	int pitch;
};

struct GameSoundOutputBuffer {
	int samplesPerSecond;
	int sampleCount;
	int16 *samples;
};

struct GameButtonState {
	int halfTransistionCount;
	bool32 endedDown;
};

struct GameControllerInput {
	bool32 isAnalog;

	real32 startX;
	real32 startY;

	real32 minX;
	real32 minY;

	real32 maxX;
	real32 maxY;

	real32 endX;
	real32 endY;

	union {
		GameButtonState buttons[6];

		struct {
			GameButtonState up;
			GameButtonState down;
			GameButtonState left;
			GameButtonState right;
			GameButtonState leftShoulder;
			GameButtonState rightShoulder;
		};
	};
};

struct GameInput {
	GameControllerInput controllers[4];
};

struct GameState {
	int toneHz;
	int blueOffset;
	int greenOffset;
};

struct GameMemory {
	bool32 isInitialized;

	uint64 permanentStorageSize;
	void *permanentStorage;

	uint64 transientStorageSize;
	void *transientStorage;
};

internal void GameUpdateAndRender(GameMemory *a_GameMemory, GameOffscreenBuffer *a_Buffer,
                                  GameSoundOutputBuffer *a_SoundBuffer, GameInput *a_Input);
