#include "handmade.h"

#include <math.h>

internal void GameOutputSound(GameSoundOutputBuffer *a_SoundBuffer,
                              int a_ToneHz) {
	local_persist real32 tSine;
	int16 toneVolume = 3000;
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
			uint8 blue = (x + a_BlueOffset);
			uint8 green = (y + a_GreenOffset);

			*pixel++ = ((green << 8) | blue);
		}

		row += a_Buffer->pitch;
	}
}

internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  GameSoundOutputBuffer *a_SoundBuffer,
                                  GameInput *a_Input) {
	local_persist auto blueOffset = 0;
	local_persist auto greenOffset = 0;
	local_persist auto toneHz = 256;

	auto input0 = &a_Input->controllers[0];
	if (input0->isAnalog) {
		blueOffset += int(4.f * (input0->endX));
		toneHz = 256 + int(128.f * (input0->endY));
	} else {
	}

	if (input0->down.endedDown) {
		greenOffset += 1;
	}

	GameOutputSound(a_SoundBuffer, toneHz);
	RenderWeirdGradient(a_Buffer, blueOffset, greenOffset);
}
