#include "handmade.cpp"
#include "handmade.h"

#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <malloc.h>
#include <stdio.h>

#include "win32_handmade.h"

global_variable bool32 g_Running;
global_variable Win32OffscreenBuffer g_Backbuffer;
global_variable LPDIRECTSOUNDBUFFER g_SecondaryBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI \
	name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DebugReadFileResult DEBUGPlatformReadEntireFile(char *a_Filename) {
	DebugReadFileResult result = {};

	auto fileHandle = CreateFileA(a_Filename, GENERIC_READ, FILE_SHARE_READ,
	                              nullptr, OPEN_EXISTING, 0, nullptr);
	if (fileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize)) {
			auto fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
			result.contents =
				VirtualAlloc(nullptr, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.contents) {
				DWORD bytesRead;
				if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, nullptr) &&
					fileSize32 == bytesRead) {
					result.contentsSize = fileSize32;
				} else {
					DEBUGPlatformFreeFileMemory(result.contents);
					result.contents = nullptr;
				}
			} else {
				// TODO: Logging
			}
		} else {
			// TODO: Logging
		}

		CloseHandle(fileHandle);
	} else {
		// TODO: Logging	
	}

	return result;
}

void DEBUGPlatformFreeFileMemory(void *a_Memory) {
	if (a_Memory) {
		VirtualFree(a_Memory, 0, MEM_RELEASE);
	}
}

bool32 DEBUGPlatformWriteEntireFile(char *a_Filename, uint32 a_MemorySize, void *a_Memory) {
	bool32 result = false;

	// IMPORTANT: Create file doesn't create a directory!
	auto fileHandle = CreateFileA(a_Filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (fileHandle != INVALID_HANDLE_VALUE) {
		DWORD bytesWritten;
		if (WriteFile(fileHandle, a_Memory, a_MemorySize, &bytesWritten, nullptr)) {
			result = bytesWritten == a_MemorySize;
		} else {
			// TODO: Logging
		}

		CloseHandle(fileHandle);
	} else {
		// TODO: Logging
	}

	return result;
}

internal void Win32LoadXInput(void) {
	auto xInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!xInputLibrary) {
		xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (!xInputLibrary) {
		xInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (xInputLibrary) {
		XInputGetState =
			reinterpret_cast<x_input_get_state *>(GetProcAddress(xInputLibrary, "XInputGetState"));
		if (!XInputGetState) {
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState =
			reinterpret_cast<x_input_set_state *>(GetProcAddress(xInputLibrary, "XInputSetState"));
		if (!XInputSetState) {
			XInputSetState = XInputSetStateStub;
		}
	} else {
	}
}

internal void Win32InitDSound(HWND a_Window, int32 a_SamplesPerSecond,
                              int32 a_BufferSize) {
	auto dSoundLibrary = LoadLibraryA("dsound.dll");
	if (dSoundLibrary) {
		auto DirectSoundCreate =
			reinterpret_cast<direct_sound_create *>(GetProcAddress(dSoundLibrary,
			                                                       "DirectSoundCreate"));

		LPDIRECTSOUND directSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(nullptr, &directSound, nullptr))) {
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = a_SamplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign =
				waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
			waveFormat.nAvgBytesPerSec =
				waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(
				directSound->SetCooperativeLevel(a_Window, DSSCL_PRIORITY))) {
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription,
					&primaryBuffer, nullptr))) {
					auto error = primaryBuffer->SetFormat(&waveFormat);
					if (SUCCEEDED(error)) {
						OutputDebugStringA("Primary buffer format was set.\n");
					} else {
					}
				} else {
				}
			} else {
			}

			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = a_BufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			auto error = directSound->CreateSoundBuffer(&bufferDescription,
			                                            &g_SecondaryBuffer, nullptr);
			if (SUCCEEDED(error)) {
				OutputDebugStringA("Secondary buffer created successfully.\n");
			}
		} else {
		}
	} else {
	}
}

internal Win32WindowDimension Win32GetWindowDimension(HWND a_Window) {
	Win32WindowDimension result;

	RECT clientRect;
	GetClientRect(a_Window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *a_Buffer, int a_Width,
                                    int a_Height) {
	if (a_Buffer->memory) {
		VirtualFree(a_Buffer->memory, 0, MEM_RELEASE);
	}

	a_Buffer->width = a_Width;
	a_Buffer->height = a_Height;

	auto bytesPerPixel = 4;

	a_Buffer->info.bmiHeader.biSize = sizeof(a_Buffer->info.bmiHeader);
	a_Buffer->info.bmiHeader.biWidth = a_Buffer->width;
	a_Buffer->info.bmiHeader.biHeight = -a_Buffer->height;
	a_Buffer->info.bmiHeader.biPlanes = 1;
	a_Buffer->info.bmiHeader.biBitCount = 32;
	a_Buffer->info.bmiHeader.biCompression = BI_RGB;

	auto bitmapMemorySize = (a_Buffer->width * a_Buffer->height) * bytesPerPixel;
	a_Buffer->memory = VirtualAlloc(nullptr, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
	                                PAGE_READWRITE);
	a_Buffer->pitch = a_Width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *a_Buffer,
                                         HDC a_DeviceContext, int a_WindowWidth,
                                         int a_WindowHeight) {
	StretchDIBits(a_DeviceContext, 0, 0, a_WindowWidth, a_WindowHeight, 0, 0,
	              a_Buffer->width, a_Buffer->height, a_Buffer->memory,
	              &a_Buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND a_Window, UINT a_Message,
                                                  WPARAM a_WParam,
                                                  LPARAM a_LParam) {
	LRESULT result = 0;

	switch (a_Message) {
		case WM_CLOSE: {
			g_Running = false;
		}
			break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		}
			break;

		case WM_DESTROY: {
			g_Running = false;
		}
			break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			Assert(!"Keyboard event came in through a non-dispatch message!");
		}
			break;

		case WM_PAINT: {
			PAINTSTRUCT Paint;
			auto DeviceContext = BeginPaint(a_Window, &Paint);
			auto Dimension = Win32GetWindowDimension(a_Window);
			Win32DisplayBufferInWindow(&g_Backbuffer, DeviceContext, Dimension.width,
			                           Dimension.height);
			EndPaint(a_Window, &Paint);
		}
			break;

		default: {
			result = DefWindowProcA(a_Window, a_Message, a_WParam, a_LParam);
		}
			break;
	}

	return result;
}

internal void Win32ClearBuffer(Win32SoundOutput *a_SoundOutput) {
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;
	if (SUCCEEDED(g_SecondaryBuffer->Lock(0, a_SoundOutput->secondaryBufferSize,
		&region1, &region1Size, &region2,
		&region2Size, 0))) {
		auto destSample = static_cast<uint8 *>(region1);
		for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
			*destSample++ = 0;
		}

		destSample = static_cast<uint8 *>(region2);
		for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
			*destSample++ = 0;
		}

		g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal void Win32FillSoundBuffer(Win32SoundOutput *a_SoundOutput,
                                   DWORD a_ByteToLock, DWORD a_BytesToWrite,
                                   GameSoundOutputBuffer *a_SourceBuffer) {
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;
	if (SUCCEEDED(g_SecondaryBuffer->Lock(a_ByteToLock, a_BytesToWrite, &region1,
		&region1Size, &region2, &region2Size,
		0))) {
		auto region1SampleCount = region1Size / a_SoundOutput->bytesPerSample;
		auto destSample = static_cast<int16 *>(region1);
		auto sourceSample = a_SourceBuffer->samples;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount;
		     ++sampleIndex) {
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++a_SoundOutput->runningSampleIndex;
		}

		auto region2SampleCount = region2Size / a_SoundOutput->bytesPerSample;
		destSample = static_cast<int16 *>(region2);
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount;
		     ++sampleIndex) {
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++a_SoundOutput->runningSampleIndex;
		}

		g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal void Win32ProcessXInputDigitalButton(DWORD a_XInputButtonState,
                                              GameButtonState *a_OldState,
                                              DWORD a_ButtonBit,
                                              GameButtonState *a_NewState) {
	a_NewState->endedDown = ((a_XInputButtonState & a_ButtonBit) == a_ButtonBit);
	a_NewState->halfTransistionCount =
		(a_OldState->endedDown != a_NewState->endedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT a_Value,
                                             SHORT a_DeadZoneThreshold) {
	real32 result = 0;
	if (a_Value < -a_DeadZoneThreshold) {
		result = real32(a_Value) / 32768.f;
	} else if (a_Value > a_DeadZoneThreshold) {
		result = real32(a_Value) / 32767.f;
	}
	return result;
}

internal void Win32ProcessKeyboardMessage(GameButtonState *a_State,
                                          bool32 a_IsDown) {
	Assert(a_State->endedDown != a_IsDown);
	a_State->endedDown = a_IsDown;
	++a_State->halfTransistionCount;
}

internal void Win32ProcessPendingMessages(GameControllerInput *a_GameControllerInput) {
	MSG message;
	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
		if (message.message == WM_QUIT) {
			g_Running = false;
		}

		switch (message.message) {
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP: {
				auto vkCode = uint32(message.wParam);
				bool32 wasDown = (message.lParam & 1 << 30) != 0;
				bool32 isDown = (message.lParam & 1 << 31) == 0;
				if (wasDown != isDown) {
					if (vkCode == 'W') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->moveUp,
							isDown);
					} else if (vkCode == 'A') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->moveLeft,
							isDown);
					} else if (vkCode == 'S') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->moveDown,
							isDown);
					} else if (vkCode == 'D') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->moveRight,
							isDown);
					} else if (vkCode == 'Q') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->leftShoulder,
							isDown);
					} else if (vkCode == 'E') {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->rightShoulder,
							isDown);
					} else if (vkCode == VK_UP) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->actionUp,
							isDown);
					} else if (vkCode == VK_LEFT) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->actionLeft,
							isDown);
					} else if (vkCode == VK_DOWN) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->actionDown,
							isDown);
					} else if (vkCode == VK_RIGHT) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->actionRight,
							isDown);
					} else if (vkCode == VK_ESCAPE) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->start,
							isDown);
					} else if (vkCode == VK_SPACE) {
						Win32ProcessKeyboardMessage(
							&a_GameControllerInput->back,
							isDown);
					}
				}

				bool32 AltKeyWasDown = message.lParam & 1 << 29;
				if (vkCode == VK_F4 && AltKeyWasDown) {
					g_Running = false;
				}
			}
				break;

			default: {
				TranslateMessage(&message);
				DispatchMessageA(&message);
			}
				break;
		}
	}
}

int CALLBACK WinMain(HINSTANCE a_Instance, HINSTANCE a_PrevInstance,
                     LPSTR a_CommandLine, int a_ShowCode) {
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	auto perfCountFrequency = perfCountFrequencyResult.QuadPart;

	Win32LoadXInput();

	WNDCLASSA windowClass = {};

	Win32ResizeDIBSection(&g_Backbuffer, 1280, 720);

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = a_Instance;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&windowClass)) {
		auto window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
		                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		                              CW_USEDEFAULT, nullptr, nullptr, a_Instance, nullptr);
		if (window) {
			HDC deviceContext = GetDC(window);
			Win32SoundOutput soundOutput = {};

			soundOutput.samplesPerSecond = 48000;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize =
				soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
			Win32InitDSound(window, soundOutput.samplesPerSecond,
			                soundOutput.secondaryBufferSize);
			Win32ClearBuffer(&soundOutput);
			g_SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			g_Running = true;

			auto samples =
				static_cast<int16 *>(VirtualAlloc(nullptr, soundOutput.secondaryBufferSize,
				                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

#if HANDMADE_INTERNAL
			auto baseAddress = LPVOID(Terabytes(uint64(2)));
#else
			LPVOID baseAddress = nullptr;
#endif

			GameMemory gameMemory = {};
			gameMemory.permanentStorageSize = Megabytes(64);
			gameMemory.transientStorageSize = Gigabytes(1);

			auto totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
			gameMemory.permanentStorage = VirtualAlloc(baseAddress, size_t(totalSize),
			                                           MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.transientStorage = static_cast<uint8 *>(gameMemory.permanentStorage) +
				gameMemory.permanentStorageSize;

			if (samples && gameMemory.permanentStorage && gameMemory.transientStorage) {
				GameInput input[2] = {};
				auto newInput = &input[0];
				auto oldInput = &input[1];

				LARGE_INTEGER lastCounter;
				QueryPerformanceCounter(&lastCounter);
				auto lastCycleCount = __rdtsc();
				while (g_Running) {
					auto oldKeyboardController = GetController(oldInput, 0);
					auto newKeyboardController = GetController(newInput, 0);;
					// TODO: Zeroing macro
					auto zeroController = GameControllerInput{};
					*newKeyboardController = zeroController;
					newKeyboardController->isConnected = true;
					for (auto buttonIndex = 0;
					     buttonIndex < ArrayCount(newKeyboardController->buttons);
					     ++buttonIndex) {
						newKeyboardController->buttons[buttonIndex].endedDown =
							oldKeyboardController->buttons[buttonIndex].endedDown;
					}

					Win32ProcessPendingMessages(newKeyboardController);

					auto maxControllerCount = DWORD(XUSER_MAX_COUNT);
					if (maxControllerCount > ArrayCount(newInput->controllers) - 1) {
						maxControllerCount = ArrayCount(newInput->controllers) - 1;
					}

					for (DWORD controllerIndex = 0;
					     controllerIndex < maxControllerCount;
					     ++controllerIndex) {
						auto ourControllerIndex = controllerIndex + 1;
						auto oldController =
							GetController(oldInput, ourControllerIndex);
						auto newController =
							GetController(newInput, ourControllerIndex);

						XINPUT_STATE controllerState;
						if (XInputGetState(controllerIndex, &controllerState) ==
							ERROR_SUCCESS) {
							newController->isConnected = true;
							auto pad = &controllerState.Gamepad;

							newController->isAnalog = true;

							newController->stickAverageX = Win32ProcessXInputStickValue(
								pad->sThumbLX,
								XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							newController->stickAverageY = Win32ProcessXInputStickValue(
								pad->sThumbLY,
								XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
								newController->stickAverageY = 1.f;
							}

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
								newController->stickAverageY = -1.f;
							}

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
								newController->stickAverageX = -1.f;
							}

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
								newController->stickAverageX = 1.f;
							}

							auto threshold = 0.5f;
							DWORD fakeMoveButtons = newController->stickAverageX < -threshold ? 1 : 0;
							Win32ProcessXInputDigitalButton(
								fakeMoveButtons, &oldController->moveLeft, 1,
								&newController->moveLeft);
							fakeMoveButtons = newController->stickAverageX > threshold ? 1 : 0;
							Win32ProcessXInputDigitalButton(
								fakeMoveButtons, &oldController->moveRight, 1,
								&newController->moveRight);
							fakeMoveButtons = newController->stickAverageY < -threshold ? 1 : 0;
							Win32ProcessXInputDigitalButton(
								fakeMoveButtons, &oldController->moveDown, 1,
								&newController->moveDown);
							fakeMoveButtons = newController->stickAverageX > threshold ? 1 : 0;
							Win32ProcessXInputDigitalButton(
								fakeMoveButtons, &oldController->moveUp, 1,
								&newController->moveUp);

							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->actionDown, XINPUT_GAMEPAD_A,
								&newController->actionDown);
							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->actionRight, XINPUT_GAMEPAD_B,
								&newController->actionRight);
							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->actionLeft, XINPUT_GAMEPAD_X,
								&newController->actionLeft);
							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->actionUp, XINPUT_GAMEPAD_Y,
								&newController->actionUp);

							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->rightShoulder,
								XINPUT_GAMEPAD_RIGHT_SHOULDER, &newController->rightShoulder);
							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->leftShoulder,
								XINPUT_GAMEPAD_LEFT_SHOULDER, &newController->leftShoulder);

							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->start,
								XINPUT_GAMEPAD_START, &newController->start);
							Win32ProcessXInputDigitalButton(
								pad->wButtons, &oldController->back,
								XINPUT_GAMEPAD_BACK, &newController->back);
						} else {
							newController->isConnected = false;
						}
					}

					DWORD byteToLock = 0;
					DWORD targetCursor;
					DWORD bytesToWrite = 0;
					DWORD playCursor;
					DWORD writeCursor;
					bool32 soundIsValid = false;
					if (SUCCEEDED(g_SecondaryBuffer->GetCurrentPosition(&playCursor,
						&writeCursor))) {
						byteToLock =
						((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
							soundOutput.secondaryBufferSize);

						targetCursor = ((playCursor + (soundOutput.latencySampleCount *
								soundOutput.bytesPerSample)) %
							soundOutput.secondaryBufferSize);
						if (byteToLock > targetCursor) {
							bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
							bytesToWrite += targetCursor;
						} else {
							bytesToWrite = targetCursor - byteToLock;
						}

						soundIsValid = true;
					}

					GameSoundOutputBuffer soundBuffer = {};
					soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
					soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
					soundBuffer.samples = samples;

					GameOffscreenBuffer buffer = {};
					buffer.memory = g_Backbuffer.memory;
					buffer.width = g_Backbuffer.width;
					buffer.height = g_Backbuffer.height;
					buffer.pitch = g_Backbuffer.pitch;

					GameUpdateAndRender(&gameMemory, &buffer, &soundBuffer, newInput);

					if (soundIsValid) {
						Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite,
						                     &soundBuffer);
					}

					auto dimension = Win32GetWindowDimension(window);
					Win32DisplayBufferInWindow(&g_Backbuffer, deviceContext,
					                           dimension.width, dimension.height);

					auto endCycleCount = __rdtsc();

					LARGE_INTEGER endCounter;
					QueryPerformanceCounter(&endCounter);

					auto cyclesElapsed = endCycleCount - lastCycleCount;
					auto counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
					auto msPerFrame =
						1000.0f * real64(counterElapsed) / real64(perfCountFrequency);
					auto fps = real64(perfCountFrequency) / real64(counterElapsed);
					auto mcpf = real64(cyclesElapsed) / (1000.0f * 1000.0f);

#if 0
                char buffer[256];
                sprintf(buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", msPerFrame, fps, mcpf);
                OutputDebugStringA(buffer);
#endif

					lastCounter = endCounter;
					lastCycleCount = endCycleCount;

					auto temp = newInput;
					newInput = oldInput;
					oldInput = temp;
				}
			} else {
			}
		} else {
		}
	} else {
	}

	return (0);
}
