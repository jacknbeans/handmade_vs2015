#include "handmade.h"

#include <math.h>

internal void GameOutputSound(GameSoundOutputBuffer *a_SoundBuffer,
                              int a_ToneHz) {
	local_persist real32 tSine;
	auto toneVolume = int16(3000);
	auto wavePeriod = a_SoundBuffer->samplesPerSecond / a_ToneHz;

	auto sampleOut = a_SoundBuffer->samples;
	for (auto sampleIndex = 0; sampleIndex < a_SoundBuffer->sampleCount;
	     ++sampleIndex) {
		auto sineValue = sinf(tSine);
		auto sampleValue = int16(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += 2.f * Pi32 * 1.0f / real32(wavePeriod);
	}
}

internal void RenderWeirdGradient(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset) {
	auto row = static_cast<uint8 *>(a_Buffer->memory);
	for (auto y = 0; y < a_Buffer->height; ++y) {
		auto pixel = reinterpret_cast<uint32 *>(row);
		for (auto x = 0; x < a_Buffer->width; ++x) {
			auto blue = uint8(x + a_BlueOffset);
			auto green = uint8(y + a_GreenOffset);

			*pixel++ = green << 8 | blue;
		}

		row += a_Buffer->pitch;
	}
}

void GameUpdateAndRender(GameMemory *a_GameMemory,
                         GameOffscreenBuffer *a_Buffer,
                         GameSoundOutputBuffer *a_SoundBuffer,
                         GameInput *a_Input) {
	Assert(sizeof(GameState) <= a_GameMemory->permanentStorageSize);

	auto gameState = reinterpret_cast<GameState *>(a_GameMemory->permanentStorage);
	if (!a_GameMemory->isInitialized) {
		char *fileName = __FILE__;

		auto file = DEBUGPlatformReadEntireFile(fileName);
		if (file.contents) {
			DEBUGPlatformWriteEntireFile("w:/handmade_vs2015/handmade_vs2015/data/test.out",
			                             file.contentsSize, file.contents);
			DEBUGPlatformFreeFileMemory(file.contents);
		}

		gameState->toneHz = 256;

		a_GameMemory->isInitialized = true;
	}

	for (auto controllerIndex = 0;
	     controllerIndex < ArrayCount(a_Input->controllers);
	     ++controllerIndex) {
		auto controller = GetController(a_Input, controllerIndex);
		if (controller->isAnalog) {
			gameState->blueOffset += int(4.f * controller->stickAverageX);
			gameState->toneHz = 256 + int(128.f * controller->stickAverageY);
		} else {
			if (controller->moveLeft.endedDown) {
				gameState->blueOffset -= 1;
			}
			if (controller->moveRight.endedDown) {
				gameState->blueOffset += 1;
			}
		}

		if (controller->actionDown.endedDown) {
			gameState->greenOffset += 1;
		}
	}

	GameOutputSound(a_SoundBuffer, gameState->toneHz);
	RenderWeirdGradient(a_Buffer, gameState->blueOffset, gameState->greenOffset);
}
