#pragma once

#include "defines.h"

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

internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  GameSoundOutputBuffer *a_SoundBuffer,
                                  GameInput *a_Input);
